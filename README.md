# vnc_mouse_touch_input_qtmcus_proxy

A man-in-the-middle VNC proxy that gives you working mouse/touch input against a VNC
server that can only *paint* — one that exports a framebuffer over RFB but has no way
to deliver the viewer's pointer events into the guest.

The proxy sits between your VNC viewer and the real VNC server. Framebuffer traffic is
relayed byte-for-byte in both directions, but the viewer→server direction is also
*tapped*: every RFB `PointerEvent` is decoded and re-emitted as a compact 6-byte binary
packet on a **separate side-channel socket**, typically an emulated UART exposed as a TCP
port. Firmware on the other end of that UART decodes the packet and injects a touch event.

```
                        ┌──────────────────────────────────────┐
   ┌──────────┐   RFB   │        VncProxyServer :5901          │   RFB   ┌──────────────┐
   │  viewer  │────────>│  ┌────────────────────────────────┐  │────────>│  device VNC  │
   │ (Remmina)│<────────│  │       VncSessionBroker         │  │<────────│    :5900     │
   └──────────┘         │  └───────────────┬────────────────┘  │         └──────────────┘
                        │        RfbClientTap│                 │
                        └────────────────────┼─────────────────┘
                                             │ 0xAA packets (write-only)
                                             v
                                     device UART :12345
```

The proxy is deliberately dumb about the RFB protocol: it never rewrites the stream, so
encodings, pixel formats and extensions are whatever the viewer and server agreed on.
It only *reads along* to find pointer events.

## Wire format of the side-channel packet

Six bytes, no checksum, big-endian coordinates. Emitted by
`RfbPointerEventPacket::setPointerData()` ([rfbpointereventpacket.cpp:9](rfbpointereventpacket.cpp#L9)):

| Offset | Size | Field        | Notes                                              |
|--------|------|--------------|----------------------------------------------------|
| 0      | 1    | sync         | always `0xAA`                                      |
| 1      | 1    | `buttonMask` | RFB button mask; bit 0 = button 1 / "pressed"      |
| 2–3    | 2    | `x`          | big-endian `uint16`, framebuffer coordinates       |
| 4–5    | 2    | `y`          | big-endian `uint16`, framebuffer coordinates       |

Example — button 1 pressed at (300, 750): `AA 01 01 2C 02 EE`

Coordinates are passed through **unscaled**. Because the viewer is looking at the same
framebuffer the device is rendering, they already match the device's screen resolution.

The channel is **host→device only**; the proxy opens the UART socket `WriteOnly` and
never reads from it ([vncsessionbroker.cpp:22](vncsessionbroker.cpp#L22)). There is no
handshake, no ack and no flow control — the receiver is expected to resync by discarding
bytes until it sees `0xAA` at packet offset 0.

## Requirements

- **Qt 6.10 or newer**, with the `Quick` module (`qt_standard_project_setup(REQUIRES 6.10)`)
- CMake ≥ 3.16 and a C++ compiler
- A VNC viewer, and a device/emulator exposing both an RFB port and a writable
  side-channel TCP port

## Building

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/Qt/6.10.2/gcc_64" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/appvnc_mouse_touch_input_qtmcus_proxy
```

Adjust `CMAKE_PREFIX_PATH` to your Qt installation. Opening `CMakeLists.txt` in Qt Creator
and picking a Qt ≥ 6.10 kit works equally well.

## Using it

1. Start the device/emulator so that its VNC port and its side-channel UART port are
   both listening.
2. Launch the proxy. Set **UART Port** to the device's side-channel port, then press
   **Listen**. The leftmost status bubble turns green once the proxy is listening on 5901.
3. Point your viewer at **`localhost:5901`** — *not* 5900. Connecting straight to 5900
   bypasses the proxy and you get a picture with no input.

With Remmina: create a new connection, protocol **VNC**, server `localhost:5901`, no
authentication. Or from a terminal:

```bash
remmina -c vnc://localhost:5901          # Remmina
vncviewer localhost:5901                 # TigerVNC
```

> **Note:** the QEMU target project reports client-side incremental-redraw ghosting with
> Remmina and recommends TigerVNC for that reason. This is a viewer-side rendering
> artifact, not a proxy bug — input injection works with either.

The four status bubbles in the header are, left to right: **proxy listening**,
**viewer connected**, **device UART socket**, **device VNC socket**. Hover for a tooltip.
Red is unconnected, yellow connecting, green connected, pink closing. All four should be
green during a healthy session. Clicking anywhere in the window dumps the current state to
the console, and the console prints `>` / `<` per relayed chunk so you can see traffic
flowing in each direction.

### Ports

| Port    | Role                                       | Set where                                         |
|---------|--------------------------------------------|---------------------------------------------------|
| `5901`  | proxy's viewer-facing RFB listener         | `proxyPort` in [Main.qml:133](Main.qml#L133)       |
| `5900`  | the device's real VNC server               | `vncPort` in [Main.qml:134](Main.qml#L134)         |
| `12345` | the device's side-channel UART             | **UART Port** spinbox in the UI                    |

Only the UART port is editable at runtime; the other two are QML property values you can
change in `Main.qml` and rebuild. The spinbox accepts 5000–65535.

Everything binds to loopback — the proxy listens on `QHostAddress::LocalHost` and dials
`127.0.0.1` for both device sockets. For a device on another machine, forward its ports to
your localhost first (e.g. `ssh -L 5900:localhost:5900 -L 12345:localhost:12345 host`).

## Session lifecycle

A `VncSessionBroker` is created per viewer connection and owns both device sockets; both
are dialled immediately on viewer connect. **Only one session exists at a time** — a new
viewer connection tears down the previous one. Either endpoint disconnecting closes the
other, so reconnecting the viewer always yields a fresh session with a reset protocol tap.

## Using it with a QEMU-emulated target

This proxy was written for a Qt for MCUs port to the Ultra96v2 (ZynqMP) running FreeRTOS on
the R5 core. QEMU emulates the ZynqMP PS DisplayPort framebuffer and exports it over RFB,
but does not emulate the I²C touch controller — so there is no path for pointer input at
all without this side channel.

Start QEMU so that it exposes both a VNC server and a UART backed by a TCP socket:

```
-vnc :0                                    # RFB on 5900
-serial tcp:localhost:12345,server,nowait  # UART1 on 12345
```

QEMU is the server on both; the proxy connects to each as a client. Set **UART Port** to
12345, press **Listen**, and point your viewer at `localhost:5901`. Because `nowait` does
not block startup, running QEMU without the proxy attached boots normally — you just get a
display you cannot touch.

On the firmware side, a task polls that UART, decodes the 6-byte packets, and calls the
same touch-event entry point the real touch controller would use. A typical single-touch
decoder only looks at **bit 0** of `buttonMask` and ignores the other mouse buttons.

## Limitations

- **Security type `None` only.** The tap steps over a single security-type selection byte
  and the ClientInit shared-desktop flag
  ([rfbclienttap.cpp:78](rfbclienttap.cpp#L78)). Any scheme where the client sends a
  response — VNC Auth's 16-byte challenge reply, for instance — desynchronises the
  parser, so leave the device's VNC server unauthenticated. By design: this is a loopback
  development tool carrying a framebuffer of a UI under test, so there is nothing to
  authenticate and nothing sensitive on the wire.
- **Unknown client messages disable the tap for the session.** On a client→server message
  type it does not recognise, `RfbClientTap` can no longer track message boundaries, so it
  sets `m_tapping = false` and degrades to a plain pass-through relay
  ([rfbclienttap.cpp:100](rfbclienttap.cpp#L100)). The picture keeps working; input
  injection stops until you reconnect. This is reported on stderr — watch for
  `Unknown RFB client message type … stopping tap`, since the symptom otherwise looks like
  a dead touchscreen rather than a protocol problem. A viewer negotiating an extension
  that adds client→server messages will trip this.
- **Pointer events only.** `KeyEvent` and `ClientCutText` are parsed for framing but
  discarded; no keyboard or clipboard is forwarded to the side channel.
- **No authentication, no encryption, loopback only.** This is a development tool.
- One viewer at a time, and the RFB and side-channel port numbers other than UART are
  compile-time values.

## Source layout

| File                        | Responsibility                                                             |
|----------------------------|-----------------------------------------------------------------------------|
| `vncproxyserver.{h,cpp}`   | `QTcpServer` on `proxyPort`; owns the viewer socket and the current session |
| `vncsessionbroker.{h,cpp}` | one session: relays both directions, dials the device VNC and UART sockets  |
| `rfbclienttap.{h,cpp}`     | incremental RFB client-stream parser; emits `pointerEvent(x, y, buttonMask)`|
| `rfbpointereventpacket.*`  | the 6-byte side-channel packet, a `QByteArray` subclass                     |
| `Main.qml`                 | port entry, Listen button, four socket-state indicator bubbles              |
| `qmlqabstractsocketforeign.h` | exposes `QAbstractSocket::SocketState` enum values to QML                 |

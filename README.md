# wayland-vr

## Overview
WaylandVR is a 3D desktop environment designed for use with stereoscopic displays (VR).
User input can be through traditional mediums, but also through hand tracking.
Since WaylandVR is strictly 3D but not strictly stereoscopic, it can be used with a normal desktop setup.
Using WebSockets as the communication method allows the renderer to be a browser window for remote access.

## Details
WaylandVR operates as a two part system, with a renderer and a server.
The server handles all of the direct application communication and sends updates to the renderer.
The renderer also recieves user input and sends that data back to the server.
The renderer should also support all major desktops (Windows, Mac, Linux) as a native application.
The server must be run on a Linux machine as it relies on the Wayland protocol for retrieving window data from applications. 
Unix domain sockets will also be supported as a communication method for systems where the renderer and the server are the same.


void toggle_STATE_FULLSCREEN() {
	XEvent ev;
	ev.xclient.type = ClientMessage;
	ev.xclient.serial = 0;
	ev.xclient.send_event = True;
	ev.xclient.display = dpy;
	ev.xclient.window = wshow;
	ev.xclient.message_type = NET_WM_STATE;
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = 2;
	ev.xclient.data.l[1] = NET_WM_STATE_FULLSCREEN;
	ev.xclient.data.l[2] = 0;
	XSendEvent(dpy, root, False, SubstructureRedirectMask |
			SubstructureNotifyMask, &ev);
	ev.xclient.message_type = NET_ACTIVE_WINDOW;
	ev.xclient.data.l[0] = 1;
	ev.xclient.data.l[1] = CurrentTime;
	ev.xclient.data.l[2] = wshow;
	XSendEvent(dpy, root, False, SubstructureRedirectMask |
			SubstructureNotifyMask, &ev);
}

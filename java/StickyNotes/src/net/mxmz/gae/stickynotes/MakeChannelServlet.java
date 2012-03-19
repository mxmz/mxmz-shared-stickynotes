package net.mxmz.gae.stickynotes;

import java.io.BufferedReader;
import java.io.IOException;

import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.google.appengine.api.channel.ChannelMessage;
import com.google.appengine.api.channel.ChannelService;
import com.google.appengine.api.channel.ChannelServiceFactory;

@SuppressWarnings("serial")
public class MakeChannelServlet extends HttpServlet {

	public void doGet(HttpServletRequest req, HttpServletResponse resp)
			throws IOException {
		String path = req.getPathInfo();
		if ( path == null || path.length() == 0 ) {
			path = "/";
		}
		ChannelService channelService = ChannelServiceFactory.getChannelService();
		String token = channelService.createChannel(path);
		resp.setContentType("text/plain");
		resp.getWriter().write(token);

	}
	public void doPost(HttpServletRequest req, HttpServletResponse resp)
			throws IOException {
		String path = req.getPathInfo();
		if ( path == null || path.length() == 0 ) {
			path = "/";
		}

		StringBuffer jb = new StringBuffer();
		String line = null;
		try {
			BufferedReader reader = req.getReader();
			while ((line = reader.readLine()) != null)
				jb.append(line);
		} catch (Exception e) { /*report an error*/ }

		ChannelService channelService = ChannelServiceFactory.getChannelService();
		channelService.sendMessage( new ChannelMessage(path, jb.toString() ) );
		resp.setContentType("text/plain");
		resp.getWriter().write("sent\n");

	}	
}

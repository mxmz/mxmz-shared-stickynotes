package net.mxmz.gae.stickynotes;

import java.io.BufferedReader;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.servlet.http.HttpServletRequest;

import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

public class Utils {

	public static Gson  makeGson() {
		return new GsonBuilder().setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_DASHES).serializeNulls().create();
	}
	public static Gson  makeGsonPretty() {
		return new GsonBuilder().setFieldNamingPolicy(FieldNamingPolicy.LOWER_CASE_WITH_DASHES).serializeNulls().setPrettyPrinting().create();
	}
	public static String readBody(final HttpServletRequest req ) {
		final StringBuffer jb = new StringBuffer();
		String line = null;
		try {
			final BufferedReader reader = req.getReader();
			while ((line = reader.readLine()) != null)
				jb.append(line);
		} catch (final Exception e) { /*report an error*/ }
		return jb.toString();
	}

	public static Map<?, ?> readJsonBody(final HttpServletRequest req) {
		return makeGson().fromJson( readBody(req), Map.class);
	}
	
	public static boolean getAttributeAsBoolean( final HttpServletRequest req, final String s ) {
			return getAttributeAsInteger(req, s) > 0;
	}
	
	public static int getAttributeAsInteger( final HttpServletRequest req, final String s ) {
		String v = req.getParameter(s);
		if ( v != null ) {
			int i = Integer.parseInt(v);
			return i;
		} else return 0;
	}

	public static Map<String, String> parseQueryString(String query)
			throws UnsupportedEncodingException {
		
		Map<String, String> params = new HashMap<String, String>();
		
		for (String param : query.split("&")) {
			String pair[] = param.split("=");
			String key = URLDecoder.decode(pair[0], "UTF-8");
			String value = "";
			if (pair.length > 1) {
				value = URLDecoder.decode(pair[1], "UTF-8");
			}
			params.put(key,value);
		}

		return params;
	}
	
	
	
}

package net.mxmz.gae.stickynotes;

import java.io.BufferedReader;
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

}

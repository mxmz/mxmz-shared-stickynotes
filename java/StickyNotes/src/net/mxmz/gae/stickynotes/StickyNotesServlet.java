package net.mxmz.gae.stickynotes;

import java.io.IOException;
import java.io.BufferedReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import javax.servlet.http.*;

import com.google.gson.FieldNamingPolicy;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;

import java.util.Date;

import com.google.appengine.api.channel.ChannelMessage;
import com.google.appengine.api.channel.ChannelService;
import com.google.appengine.api.channel.ChannelServiceFactory;
import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.PreparedQuery;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.repackaged.com.google.common.collect.Lists;

import net.mxmz.gae.stickynotes.Utils;

@SuppressWarnings("serial")
public class StickyNotesServlet extends HttpServlet {

	/*
	private String makeKeyPath(Key k, String separator) {
			Key parent = k.getParent();
			return ( parent != null ? makeKeyPath(parent, separator) + separator : "" ) +
					k.getId(); 
	}
	
	
	private String makeFullName( Datastore datastore, Entity e ) {
		Key parent = e.getParent();
		String name = (String)e.getProperty(Datastore.NAME_PROP);
		if ( parent != null ) {
			return makeFullName(datastore, datastore.getEntity(parent) ) + "/" + name;
		} else {
			return name;
		}
	}
	
	private HashMap<String,Object> makeEntityMap( Datastore datastore, HashMap<String,Object> root, Entity e, boolean putEntity ) {
		Key parent = e.getParent();
		String name = (String)e.getProperty(Datastore.NAME_PROP);
		if ( parent != null ) {
			root = makeEntityMap(datastore, root, datastore.getEntity(parent), false );
		}
		
		HashMap<String,Object> entries = (HashMap<String,Object>)root.get("entries");
		if ( entries == null ) {
			entries = new HashMap<String,Object>();
			root.put("entries",entries);
		}
		HashMap<String,Object> entity = (HashMap<String,Object> )entries.get(name);
		if ( entity == null ) {
			entity = putEntity ? entityToMap(datastore,e) : new HashMap<String,Object>();
		}
		entries.put(name,entity);
		return entity;
	}
	*/
	
	private HashMap<String,Object> entityToMap(Entity e, String pname) {
		HashMap<String,Object> m = new HashMap<String,Object>();
		m.put("name", (String)e.getProperty(Datastore.NAME_PROP) );
		m.put("folder", pname );
		m.put("data", Utils.makeGson().fromJson( (String)e.getProperty(Datastore.DATA_PROP), Map.class ) );
		m.put("ctime", ((Date)e.getProperty(Datastore.CTIME_PROP) ).getTime()/1000 );
		m.put("id", e.getProperty(Datastore.ID_PROP) );
		m.put("pid", e.getProperty(Datastore.PID_PROP) );
		return m;
	}

	public void doGet(HttpServletRequest req, HttpServletResponse resp)
			throws IOException {
		String path = req.getPathInfo();
		if ( path == null || path.length() == 0 ) {
			path = "/PUBLIC";
		}
		path = path.substring(1);
		ArrayList<HashMap> results= new ArrayList<HashMap>();
		//HashMap results = new HashMap();
		String[] patharray = path.split("/") ;
		List<String> pathlist = Lists.reverse(Arrays.asList( patharray  ) );
		Datastore datastore = new Datastore();
		if ( path.endsWith("/") ) {
			Datastore.EntityWrap  parent = datastore.getEntity(pathlist, false);
			if ( ! parent.isNull() ) {
				Datastore.EntityIterableWrap it = datastore.getEntityChildren(parent);
				for (Entity e: it.iterator ) {
					//makeEntityMap(datastore, results, e, true);
					results.add( entityToMap(e,parent.getPath()));
					//				}
				}
				
			}
		} else {
			Datastore.EntityWrap  e = datastore.getEntity(pathlist, false);
			if ( e.entity != null ) {
				results.add(entityToMap(e.entity, e.getParentPath()));
				//makeEntityMap(datastore, results, e, true);
			}
		}

		resp.setContentType("application/json");
		//String msg = Utils.makeGsonPretty().toJson(list, ArrayList.class);
		String msg = Utils.makeGsonPretty().toJson(results, ArrayList.class);
		resp.getWriter().println(msg);
	}

	public void doPost(HttpServletRequest req, HttpServletResponse resp)
			throws IOException {
		String path = req.getPathInfo();
		boolean meta = Integer.getInteger( (String)req.getAttribute("meta") ) > 0;
		if ( path == null || path.length() == 0 ) {
			path = "/PUBLIC";
		}
		path = path.substring(1);
		//String body = Utils.readBody(req);
		Map<?,?> input = Utils.readJsonBody(req);
		List<String> pathlist = Lists.reverse(Arrays.asList( path.split("/") ) );
		Datastore datastore = new Datastore();
		Datastore.EntityWrap e = datastore.updateEntity(pathlist, input, meta);
		
		ChannelService channelService = ChannelServiceFactory.getChannelService();
		channelService.sendMessage( new ChannelMessage("/" + path, "M" ) );
		channelService.sendMessage( new ChannelMessage(e.getParentPath()+"/", "M" ) );
				
		
		resp.setContentType("application/json");
		String msg = Utils.makeGson().toJson(entityToMap(e.entity,e.getParentPath()), Map.class );
		resp.getWriter().println(msg);
	}
}


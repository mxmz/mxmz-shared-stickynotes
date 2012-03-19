package net.mxmz.gae.stickynotes;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.google.appengine.api.datastore.DatastoreService;
import com.google.appengine.api.datastore.DatastoreServiceFactory;
import com.google.appengine.api.datastore.Entity;
import com.google.appengine.api.datastore.EntityNotFoundException;
import com.google.appengine.api.datastore.PreparedQuery;
import com.google.appengine.api.datastore.Query;
import com.google.appengine.api.datastore.Key;
import com.google.appengine.api.datastore.KeyFactory;

public class Datastore {	
	private DatastoreService datastore = DatastoreServiceFactory.getDatastoreService();

	static String KIND_NAME = "StickyNote";
	static String NAME_PROP = "name";
	static String CTIME_PROP= "ctime";
	static String DATA_PROP	= "data";
	static String META_PROP	= "meta";
	static String ID_PROP	= "id";
	static String PID_PROP	= "pid";
	
	class EntityWrap {
		public final Entity entity;
		public final EntityWrap parent;
		public EntityWrap( Entity myself, EntityWrap parent ) {
				this.entity = myself;
				this.parent = parent;
		}
		boolean isNull() { return entity == null; }
		
		String getParentPath() {
			return  ( parent != null ?  parent.getPath() : "" );
		}
		String getPath() {
			return  ( getParentPath() ) + "/" + 
					(String)entity.getProperty(NAME_PROP);
		}
	
	};
	
	class EntityIterableWrap {
		final Iterable<Entity> iterator;
		final EntityWrap parent;
		public EntityIterableWrap( Iterable<Entity> iterator, EntityWrap parent ) {
			this.iterator = iterator;
			this.parent = parent;
		}
	};

	public EntityWrap updateEntity( List<String> path, Map data, boolean meta ) {
		EntityWrap e = getEntity(path, true);
		if ( meta ) {
			e.entity.setProperty(META_PROP, Utils.makeGson().toJson(data, Map.class ) );
		} else {
			e.entity.setProperty(DATA_PROP, Utils.makeGson().toJson(data, Map.class ) );
		}
		this.datastore.put(e.entity);
		return e;
	}
	/*
	public Entity	getEntity( Key k) {			
		Entity e = null;
		try {
			e = datastore.get(k);
		}
		catch( EntityNotFoundException error)
		{
			//
			e = null;
		}
		return e;
	}

	public Entity	getEntity( long id) {			
		//Query q = new Query(KIND_NAME);
		Key k = KeyFactory.createKey(KIND_NAME, id);
		Entity e = null;
		try {
			e = datastore.get(k);
		}
		catch( EntityNotFoundException error)
		{
			//
			e = null;
		}
		//q.addFilter(Entity.KEY_RESERVED_PROPERTY, Query.FilterOperator.EQUAL, id);
		//PreparedQuery pq = this.datastore.prepare(q);
		//Entity e = pq.asSingleEntity();
		return e;
	}
	
	*/
	
	public EntityIterableWrap getEntityChildren( EntityWrap parent ) {
		Query q = new Query(KIND_NAME);
		String pid = (String)parent.entity.getProperty(ID_PROP);
		q.addFilter( PID_PROP, Query.FilterOperator.EQUAL, pid);
		PreparedQuery pq = this.datastore.prepare(q);
		return new EntityIterableWrap( pq.asIterable(), parent );
	}

	public EntityWrap getEntity( List<String> path, boolean autoinsert )
	{	
		EntityWrap current = null;
		
		for ( int i = 0 ; i < path.size(); ++i ) {
			String name = path.get(i);
			String pid = null;
			Query q = new Query(KIND_NAME);
			q.addFilter( NAME_PROP, Query.FilterOperator.EQUAL, name );
			if ( current != null ) {
				pid = (String)current.entity.getProperty(ID_PROP);
				q.addFilter( PID_PROP, Query.FilterOperator.EQUAL, pid);

			}
			PreparedQuery pq = this.datastore.prepare(q);
			Entity e = pq.asSingleEntity();

			if ( e == null && autoinsert ) {
				e = new Entity( KIND_NAME);
				Date now = new Date();
				e.setProperty(NAME_PROP, name );
				e.setProperty(CTIME_PROP, now);
				e.setProperty(ID_PROP, java.util.UUID.randomUUID().toString() );
				e.setProperty(PID_PROP, pid );
				this.datastore.put(e);
			}
			current = new EntityWrap(e,current);
		}
		return current;
	}
		
}


/*

		if ( name.matches("^[0-9].*")) { // search by Id
				long id = Long.parseLong(name);
				Key k = KeyFactory.createKey(parent.getKey(), KIND_NAME,id);
				try {
					e = datastore.get(k);
				}
				catch( EntityNotFoundException error)
				{	autoinsert = false;
				}
			} else {
	

if ( name.matches("^[0-9].*")) { // search by Id ...
	long id = Long.parseLong(name);
	Key k = KeyFactory.createKey(KIND_NAME, id);
	try {
		e = datastore.get(k);
	}
	catch( EntityNotFoundException error)
	{	autoinsert = false;
	}
} else {






*/

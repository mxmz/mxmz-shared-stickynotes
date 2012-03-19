
function randomString() {
	var chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXTZabcdefghiklmnopqrstuvwxyz";
	var string_length = 8;
	var randomstring = '';
	for (var i=0; i<string_length; i++) {
		var rnum = Math.floor(Math.random() * chars.length);
		randomstring += chars.substring(rnum,rnum+1);
	}
	return randomstring;
}

function getQueryVariable(query, variable) {
        var vars = query.split("&");
        for (var i = 0; i < vars.length; i++) {
            var pair = vars[i].split("=");
            if (pair[0] == variable) {
                return decodeURIComponent( pair[1] );
            }
        }
        alert('Query Variable ' + variable + ' not found');
    }

function getQueryVariableDefault(query, variable, default_value ) {
        var vars = query.split("&");
        for (var i = 0; i < vars.length; i++) {
            var pair = vars[i].split("=");
            if (pair[0] == variable) {
                return decodeURIComponent( pair[1] );
            }
        }
	return default_value;
    }



function createCookie(name,value,days) {
	if (days) {
		var date = new Date();
		date.setTime(date.getTime()+(days*24*60*60*1000));
		var expires = "; expires="+date.toGMTString();
	}
	else var expires = "";
	document.cookie = name+"="+encodeURIComponent(value)+expires+"; path=/";
}

function readCookie(name) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0;i < ca.length;i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return decodeURIComponent( c.substring(nameEQ.length,c.length) );
	}
	return "";
}

function readCookieWithDefault(name,default_value) {
	var nameEQ = name + "=";
	var ca = document.cookie.split(';');
	for(var i=0;i < ca.length;i++) {
		var c = ca[i];
		while (c.charAt(0)==' ') c = c.substring(1,c.length);
		if (c.indexOf(nameEQ) == 0) return decodeURIComponent( c.substring(nameEQ.length,c.length) );
	}
	return default_value || "";
}

function readIntegerCookieWithDefault(name,default_value) {
		var v = parseInt( readCookieWithDefault(name,"") );
		return isNaN(v) ? default_value  : v;
}


function readCookieAndErase(name) {
	var val = readCookie(name);
	eraseCookie(name);
	return val;
}

function eraseCookie(name) {
	createCookie(name,"",-1);
}



function readQueryValues(query ) {
	var data = { };
        var vars = query.split("&");
        for (var i = 0; i < vars.length; i++) {
	    if ( vars[i].length > 0 ) {
	            var pair = vars[i].split("=");
        	    data[ decodeURIComponent(pair[0])]  = decodeURIComponent( pair[1] || "" );
		}
        }
	return data;
    }

function makeQueryString(data) {
	var qstring = ""
	for( var n in data ) {
		qstring += encodeURIComponent(n) + "=" + encodeURIComponent(data[n]||"") + "&"
	}
    
    if ( qstring.length == 0 ) { qstring = "1=1"}
	return qstring;
}


var hash_values_cache = null; 

function getHashValueDefault( key, default_value ) {
	hash_values_cache = hash_values_cache || readQueryValues(document.location.hash.substr(1))
	return hash_values_cache[ key ] || default_value;
}


function setHashValue( key, value ) {
	hash_values_cache = hash_values_cache || readQueryValues(document.location.hash.substr(1))
	hash_values_cache[key] = value;
        document.location.hash = makeQueryString(hash_values_cache);
}

function deleteHashValue( key ) {
	delete hash_values_cache[key];
    document.location.hash = makeQueryString(hash_values_cache);
}


function getIntegerHashValueDefault(name,default_value) {
		var v = parseInt( getHashValueDefault(name,"") );
		return isNaN(v) ? default_value  : v;
}




var hideandshow_cache = {} 

function hideAndShow(selHide, selShow) {
    $(selHide).filter(":visible").slideUp( 200, function() { $(selShow).slideDown(200); });

    hideandshow_cache[selHide] = selShow;
    var cookie = "";
    for ( var k in hideandshow_cache ) {
            cookie += k + "~" + hideandshow_cache[k]+ '~~';
    }
    createCookie('hideandshow', cookie );
 }


function hideAndShow_init() {

        $(readCookieWithDefault('hideandshow','').split(/~~/) ).each( function() {
                            var tokens = this.split(/~/);
                            if ( tokens.length == 2 ) {
                                $( tokens[0]).hide();
                                $( tokens[1]).show();
                                hideandshow_cache[tokens[0]] = tokens[1];
                            }
                        });
}



























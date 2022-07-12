#include "elektra-xfconf-channel.h"
#include "elektra-xfconf-util.h"
#include "elektra-xfconf.h"

#include <gelektra-key.h>

typedef struct XfconfCache XfconfCache;
struct _XfconfChannel
{
	GObject parent;
	gchar * channel_name;
};

typedef struct XfconfChannelClass
{
	GObjectClass parent;

	void (*property_changed) (XfconfChannel * channel, const gchar * property, const GValue * value);
} XfconfChannelClass;

enum
{
	SIG_PROPERTY_CHANGED = 0,
	N_SIGS,
};

enum
{
	PROP0 = 0,
	PROP_CHANNEL_NAME,
};


static GObject * xfconf_channel_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties);
static void xfconf_channel_set_g_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void xfconf_channel_get_g_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void xfconf_channel_dispose (GObject * obj);
static void xfconf_channel_finalize (GObject * obj);

static void xfconf_channel_property_changed (XfconfCache * cache, const gchar * channel_name, const gchar * property, const GValue * value,
					     gpointer user_data);

static guint signals[N_SIGS] = {
	0,
};

G_DEFINE_TYPE (XfconfChannel, xfconf_channel, G_TYPE_OBJECT)


static void xfconf_channel_class_init (XfconfChannelClass * klass)
{
	trace ();
	GObjectClass * object_class = (GObjectClass *) klass;

	object_class->constructor = xfconf_channel_constructor;
	object_class->set_property = xfconf_channel_set_g_property;
	object_class->get_property = xfconf_channel_get_g_property;
	object_class->dispose = xfconf_channel_dispose;
	object_class->finalize = xfconf_channel_finalize;

	signals[SIG_PROPERTY_CHANGED] = g_signal_new (I_ ("property-changed"), XFCONF_TYPE_CHANNEL, G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
						      G_STRUCT_OFFSET (XfconfChannelClass, property_changed), NULL, NULL, NULL, G_TYPE_NONE,
						      2, G_TYPE_STRING, G_TYPE_VALUE);

	g_object_class_install_property (object_class, PROP_CHANNEL_NAME,
					 g_param_spec_string ("channel-name", "Channel Name", "The name of the channel", NULL,
							      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY | G_PARAM_STATIC_NAME |
								      G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB));
}

static void xfconf_channel_init (XfconfChannel * instance)
{
	unimplemented ();
}

static GObject * xfconf_channel_constructor (GType type, guint n_construct_properties, GObjectConstructParam * construct_properties)
{
	trace ();
	const gchar * channel_name = NULL;
	guint i;
	XfconfChannel * channel = NULL;

	for (i = 0; i < n_construct_properties; ++i)
	{
		if (!strcmp (g_param_spec_get_name (construct_properties[i].pspec), "channel-name"))
			channel_name = g_value_get_string (construct_properties[i].value);
	}

	if (G_UNLIKELY (!channel_name))
	{
		g_warning ("Assertion 'channel_name != NULL' failed");
		return NULL;
	}

	channel = XFCONF_CHANNEL (
		G_OBJECT_CLASS (xfconf_channel_parent_class)->constructor (type, n_construct_properties, construct_properties));
	return G_OBJECT (channel);
}

static void xfconf_channel_set_g_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec)
{
	trace ();
	XfconfChannel * channel = XFCONF_CHANNEL (object);

	if (property_id == PROP_CHANNEL_NAME)
	{
		g_assert (channel->channel_name == NULL);
		channel->channel_name = g_value_dup_string (value);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void xfconf_channel_get_g_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec)
{
	trace ();
	XfconfChannel * channel = XFCONF_CHANNEL (object);

	if (property_id == PROP_CHANNEL_NAME)
	{
		g_value_set_string (value, channel->channel_name);
	}
	else
	{
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void xfconf_channel_dispose (GObject * obj)
{
	trace ();
	g_debug ("xfconf_channel_dispose");
}

static void xfconf_channel_finalize (GObject * obj)
{
	trace ();
	XfconfChannel * channel = XFCONF_CHANNEL (obj);
	g_free (channel->channel_name);
	G_OBJECT_CLASS (xfconf_channel_parent_class)->finalize (obj);
}


static gint find_by_name (gconstpointer channel, gconstpointer name)
{
	trace ();
	const XfconfChannel * xfconf_channel = channel;
	return strcmp (xfconf_channel->channel_name, name);
}

XfconfChannel * xfconf_channel_get (const gchar * channel_name)
{
	trace ();
	GList * channel_item = g_list_find_custom (channel_list, channel_name, &find_by_name);
	if (channel_item != NULL)
	{
		return channel_item->data;
	}
	return xfconf_channel_new (channel_name);
}

static gint compare_channel (gconstpointer a, gconstpointer b)
{
	const XfconfChannel * channel_a = a;
	const XfconfChannel * channel_b = b;
	return strcmp (channel_a->channel_name, channel_b->channel_name);
}

XfconfChannel * xfconf_channel_new (const gchar * channel_name)
{
	trace ();
	XfconfChannel * channel = g_object_new (XFCONF_TYPE_CHANNEL, "channel-name", channel_name, NULL);
	channel_list = g_list_insert_sorted (channel_list, channel, &compare_channel);
	return channel;
}

XfconfChannel * xfconf_channel_new_with_property_base (const gchar * channel_name, const gchar * property_base)
{
	unimplemented ();
}

static GElektraKeySet * keySet_from_channel (const gchar * channel_name)
{
	trace ();
	gchar * key_name = malloc ((strlen (channel_name) + strlen (XFCONF_ROOT) + 2) * sizeof (char));
	sprintf (key_name, "%s/%s", XFCONF_ROOT, channel_name);
	GElektraKeySet * key_set = gelektra_keyset_new (0, GELEKTRA_KEYSET_END);
	GElektraKey * parent_key = gelektra_key_new ("/", GELEKTRA_KEY_END);
	g_debug ("Fetch keys from parent: %s", key_name);
	switch (gelektra_kdb_get (gElektraKdb, key_set, parent_key))
	{
	case -1:
		g_warning ("There was a failure fetching the keys");
		break;
	case 0:
		g_debug ("The keyset did not change");
		break;
	case 1:
		g_debug ("Retrieved the keyset");
		break;
	default:
		g_error ("An unknown error occurred during keyset fetch");
		break;
	}
	free (key_name);
	return key_set;
}

gboolean xfconf_channel_has_property (XfconfChannel * channel, const gchar * property)
{
	GElektraKeySet * key_set = keySet_from_channel (channel->channel_name);
	return FALSE;
}

gboolean xfconf_channel_is_property_locked (XfconfChannel * channel, const gchar * property)
{
	unimplemented ();
}

void xfconf_channel_reset_property (XfconfChannel * channel, const gchar * property_base, gboolean recursive)
{
	unimplemented ();
}

GHashTable * xfconf_channel_get_properties (XfconfChannel * channel, const gchar * property_base)
{
	unimplemented ();
}

/* basic types */

gchar * xfconf_channel_get_string (XfconfChannel * channel, const gchar * property, const gchar * default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_string (XfconfChannel * channel, const gchar * property, const gchar * value)
{
	unimplemented ();
}

gint32 xfconf_channel_get_int (XfconfChannel * channel, const gchar * property, gint32 default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_int (XfconfChannel * channel, const gchar * property, gint32 value)
{
	unimplemented ();
}

guint32 xfconf_channel_get_uint (XfconfChannel * channel, const gchar * property, guint32 default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_uint (XfconfChannel * channel, const gchar * property, guint32 value)
{
	unimplemented ();
}

guint64 xfconf_channel_get_uint64 (XfconfChannel * channel, const gchar * property, guint64 default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_uint64 (XfconfChannel * channel, const gchar * property, guint64 value)
{
	unimplemented ();
}

gdouble xfconf_channel_get_double (XfconfChannel * channel, const gchar * property, gdouble default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_double (XfconfChannel * channel, const gchar * property, gdouble value)
{
	unimplemented ();
}

gboolean xfconf_channel_get_bool (XfconfChannel * channel, const gchar * property, gboolean default_value)
{
	unimplemented ();
}
gboolean xfconf_channel_set_bool (XfconfChannel * channel, const gchar * property, gboolean value)
{
	unimplemented ();
}

/* this is just convenience API for the array stuff, where
 * all the values are G_TYPE_STRING */
gchar ** xfconf_channel_get_string_list (XfconfChannel * channel, const gchar * property)
{
	unimplemented ();
}
gboolean xfconf_channel_set_string_list (XfconfChannel * channel, const gchar * property, const gchar * const * values)
{
	unimplemented ();
}

/* really generic API - can set some value types that aren't
 * supported by the basic type API, e.g., char, signed short,
 * unsigned int, etc.  no, you can't set arbitrary GTypes. */
gboolean xfconf_channel_get_property (XfconfChannel * channel, const gchar * property, GValue * value)
{
	trace ();
	GElektraKeySet * key_set = keySet_from_channel (channel->channel_name);
	gchar * property_name = malloc ((strlen (XFCONF_ROOT) + strlen (channel->channel_name) + 2) * sizeof (char));
	sprintf (property_name, "%s/%s%s", XFCONF_ROOT, channel->channel_name, property);
	g_debug ("request key %s with type %lu, on channel: %s", property, value->g_type, channel->channel_name);
	Key * cur;
	KeySet * toWriteKS;
	Key * toWrite;
	g_debug ("channel has %zd keys", ksGetSize (key_set->keyset));
	for (elektraCursor it = 0; it < ksGetSize (key_set->keyset); ++it)
	{
		cur = ksAtCursor (key_set->keyset, it);
		g_debug ("found key: %s", keyName (cur));
	}
	GElektraKey * key = gelektra_keyset_lookup_byname (key_set, property_name, GELEKTRA_KDB_O_NONE);
	if (key == NULL)
	{
		g_debug ("got null from keyset by looking up %s", property_name);
		return FALSE;
	}
	value->g_type = G_TYPE_STRING;
	const gchar * key_value = gelektra_key_string (key);
	g_debug ("Found value %s to key %s", key_value, property_name);
	g_value_set_string (value, key_value);
	free (property_name);
	return TRUE;
}
gboolean xfconf_channel_set_property (XfconfChannel * channel, const gchar * property, const GValue * value)
{
	trace ();
	gchar * property_name = malloc ((strlen (XFCONF_ROOT) + strlen (channel->channel_name) + 2) * sizeof (char));
	sprintf (property_name, "%s/%s%s", XFCONF_ROOT, channel->channel_name, property);
	g_debug ("set key %s with type %lu, on channel: %s", property, value->g_type, channel->channel_name);
	GElektraKey * key = gelektra_key_new (property_name, GELEKTRA_KEY_END);
	g_debug ("set %s to %s", property_name, (gchar *) value->data->v_pointer);
	return gelektra_key_setstring (key, value->data->v_pointer) > 0;
}

/* array types - arrays can be made up of values of arbitrary
 * (and mixed) types, even some not supported by the basic
 * type API */

gboolean xfconf_channel_get_array (XfconfChannel * channel, const gchar * property, GType first_value_type, ...)
{
	unimplemented ();
}
gboolean xfconf_channel_get_array_valist (XfconfChannel * channel, const gchar * property, GType first_value_type, va_list var_args)
{
	unimplemented ();
}
GPtrArray * xfconf_channel_get_arrayv (XfconfChannel * channel, const gchar * property)
{
	unimplemented ();
}

gboolean xfconf_channel_set_array (XfconfChannel * channel, const gchar * property, GType first_value_type, ...)
{
	unimplemented ();
}
gboolean xfconf_channel_set_array_valist (XfconfChannel * channel, const gchar * property, GType first_value_type, va_list var_args)
{
	unimplemented ();
}
gboolean xfconf_channel_set_arrayv (XfconfChannel * channel, const gchar * property, GPtrArray * values)
{
	unimplemented ();
}

/* struct types */

gboolean xfconf_channel_get_named_struct (XfconfChannel * channel, const gchar * property, const gchar * struct_name, gpointer value_struct)
{
	unimplemented ();
}
gboolean xfconf_channel_set_named_struct (XfconfChannel * channel, const gchar * property, const gchar * struct_name, gpointer value_struct)
{
	unimplemented ();
}

gboolean xfconf_channel_get_struct (XfconfChannel * channel, const gchar * property, gpointer value_struct, GType first_member_type, ...)
{
	unimplemented ();
}
gboolean xfconf_channel_get_struct_valist (XfconfChannel * channel, const gchar * property, gpointer value_struct, GType first_member_type,
					   va_list var_args)
{
	unimplemented ();
}
gboolean xfconf_channel_get_structv (XfconfChannel * channel, const gchar * property, gpointer value_struct, guint n_members,
				     GType * member_types)
{
	unimplemented ();
}

gboolean xfconf_channel_set_struct (XfconfChannel * channel, const gchar * property, const gpointer value_struct, GType first_member_type,
				    ...)
{
	unimplemented ();
}
gboolean xfconf_channel_set_struct_valist (XfconfChannel * channel, const gchar * property, const gpointer value_struct,
					   GType first_member_type, va_list var_args)
{
	unimplemented ();
}
gboolean xfconf_channel_set_structv (XfconfChannel * channel, const gchar * property, const gpointer value_struct, guint n_members,
				     GType * member_types)
{
	unimplemented ();
}

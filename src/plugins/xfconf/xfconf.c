/**
 * @file
 *
 * @brief Source for xfconf plugin
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 *
 */

#include "xfconf.h"
#include "kdbease.h"

#include <kdbhelper.h>
#include <kdblogger.h>
#include <libgen.h>
#include <xfconf/xfconf.h>

int elektraXfconfOpen (Plugin * handle ELEKTRA_UNUSED, Key * errorKey ELEKTRA_UNUSED)
{
	ELEKTRA_LOG ("try to initialize xfconf\n");
	GError * err = NULL;
	if (xfconf_init (&err))
	{
		ELEKTRA_LOG_DEBUG ("succeed initielize xfconf\n");
		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	else
	{
		ELEKTRA_LOG ("unable to initialize xfconf(%d): %s\n", err->code, err->message);
		g_error_free (err);
		return ELEKTRA_PLUGIN_STATUS_ERROR;
	}
}

int elektraXfconfClose (Plugin * handle ELEKTRA_UNUSED, Key * errorKey ELEKTRA_UNUSED)
{
	xfconf_shutdown ();
	return ELEKTRA_PLUGIN_STATUS_SUCCESS;
}

int elektraXfconfGet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned, Key * parentKey)
{
	ELEKTRA_LOG ("issued get\n");
	if (!elektraStrCmp (keyName (parentKey), "system:/elektra/modules/xfconf"))
	{
		ELEKTRA_LOG_DEBUG ("getting system modules values\n");
		KeySet * contract =
			ksNew (30, keyNew ("system:/elektra/modules/xfconf", KEY_VALUE, "xfconf plugin waits for your orders", KEY_END),
			       keyNew ("system:/elektra/modules/xfconf/exports", KEY_END),
			       keyNew ("system:/elektra/modules/xfconf/exports/open", KEY_FUNC, elektraXfconfOpen, KEY_END),
			       keyNew ("system:/elektra/modules/xfconf/exports/close", KEY_FUNC, elektraXfconfClose, KEY_END),
			       keyNew ("system:/elektra/modules/xfconf/exports/get", KEY_FUNC, elektraXfconfGet, KEY_END),
			       keyNew ("system:/elektra/modules/xfconf/exports/set", KEY_FUNC, elektraXfconfSet, KEY_END),
#include ELEKTRA_README
			       keyNew ("system:/elektra/modules/xfconf/infos/version", KEY_VALUE, PLUGINVERSION, KEY_END), KS_END);
		ksAppend (returned, contract);
		ksDel (contract);

		return ELEKTRA_PLUGIN_STATUS_SUCCESS;
	}
	// get all keys

	// todo: remove workaround which requires a channel to exist as a file
	char * absolutePath = elektraStrDup (keyString (parentKey));
	const char * channelName = basename (absolutePath);
	const char * parentName = keyName (parentKey);
	ELEKTRA_LOG_DEBUG ("fetch keys from channel: %s\n", channelName);
	Key * channelKey = keyDup (parentKey, KEY_CP_NAME);
	keySetMeta (channelKey, "channel", channelName);
	ELEKTRA_LOG_DEBUG ("appended meta-key channel in %s (%s)\n", keyName (channelKey), channelName);
	ksAppendKey (returned, channelKey);
	XfconfChannel * channel = xfconf_channel_get (channelName);
	if (channel == NULL)
	{
		ELEKTRA_LOG_DEBUG ("retrieved NULL attempting getting channel: %s\n", channelName);
	}
	GHashTable * properties = xfconf_channel_get_properties (channel, NULL);
	if (properties == NULL)
	{
		ELEKTRA_LOG_DEBUG ("retrieved NULL attempting getting properties\n");
	}
	GList * channelKeys = g_hash_table_get_keys (properties);
	while (channelKeys != NULL)
	{
		char * keyName = elektraStrDup (channelKeys->data);
		GValue keyValue = G_VALUE_INIT;
		g_value_init (&keyValue, G_TYPE_STRING);
		xfconf_channel_get_property (channel, keyName, &keyValue);
		ELEKTRA_LOG_DEBUG ("found %s -> %s\n", keyName, g_value_get_string (&keyValue));
		char * absoluteKeyName = elektraMalloc ((elektraStrLen (keyName) + elektraStrLen (parentName)) * sizeof (char *));
		absoluteKeyName[0] = '\0';
		strcat (absoluteKeyName, parentName);
		strcat (absoluteKeyName, keyName);
		Key * key = keyNew (absoluteKeyName, KEY_END);
		keySetString (key, g_value_get_string (&keyValue));
		ksAppendKey (returned, key);
		channelKeys = channelKeys->next;
	}
	g_list_free (channelKeys);
	g_hash_table_destroy (properties);
	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

int elektraXfconfSet (Plugin * handle ELEKTRA_UNUSED, KeySet * returned ELEKTRA_UNUSED, Key * parentKey ELEKTRA_UNUSED)
{
	ELEKTRA_LOG_DEBUG ("issued set with parent %s\n", keyName (parentKey));
	const char * parentName = keyName (parentKey);
	const Key * channelKey = keyGetMeta (ksLookupByName (returned, parentName, KDB_O_NONE), "channel");
	ELEKTRA_LOG_DEBUG ("channel key: %d\n", channelKey == NULL);
	const char * channelName = keyString (channelKey);
	ELEKTRA_LOG_DEBUG ("using channel %s of parent %s\n", channelName, parentName);

	XfconfChannel * channel = xfconf_channel_get (channelName);
	if (channel == NULL)
	{
		ELEKTRA_LOG_DEBUG ("retrieved NULL attempting getting channel: %s\n", channelName);
	}

	for (elektraCursor it = 0; it < ksGetSize (returned); ++it)
	{
		Key * cur = ksNext (returned);
		const char * keyName = elektraKeyGetRelativeName (cur, parentKey);
		if (keyName == NULL)
		{
			// happens for the root key which holds the channel name
			ELEKTRA_LOG_DEBUG ("keyName is null!\n");
			continue;
		}
		char * xfconfKeyName = elektraMalloc ((elektraStrLen (keyName) + 2) * sizeof (char *));
		xfconfKeyName[0] = '/';
		strncpy (&xfconfKeyName[1], keyName, elektraStrLen (keyName));
		ELEKTRA_LOG_DEBUG ("setting key %s to %s\n", xfconfKeyName, keyString (cur));
		GValue keyValue = G_VALUE_INIT;
		if (!xfconf_channel_get_property (channel, xfconfKeyName, &keyValue))
		{
			ELEKTRA_LOG_DEBUG ("key was not found, initialize a new one of type string\n");
			g_value_init (&keyValue, G_TYPE_STRING);
		}
		ELEKTRA_LOG_DEBUG ("key is of type: %lu\n", keyValue.g_type);
		g_value_set_string (&keyValue, keyString (cur));
		if (!xfconf_channel_set_property (channel, xfconfKeyName, &keyValue))
		{
			ELEKTRA_LOG_DEBUG ("unable to set value\n");
		}
	}
	return ELEKTRA_PLUGIN_STATUS_NO_UPDATE;
}

Plugin * ELEKTRA_PLUGIN_EXPORT
{
	// clang-format off
	return elektraPluginExport ("xfconf",
		ELEKTRA_PLUGIN_OPEN,	&elektraXfconfOpen,
		ELEKTRA_PLUGIN_CLOSE,	&elektraXfconfClose,
		ELEKTRA_PLUGIN_GET,	&elektraXfconfGet,
		ELEKTRA_PLUGIN_SET,	&elektraXfconfSet,
		ELEKTRA_PLUGIN_END);
}

/**
 * @file
 *
 * @brief Private declarations.
 *
 * @copyright BSD License (see LICENSE.md or https://www.libelektra.org)
 */

#ifndef KDBPRIVATE_H
#define KDBPRIVATE_H

#include <elektra.h>
#include <elektra/error.h>
#include <kdb.h>
#include <kdbextension.h>
#include <kdbhelper.h>
#include <kdbio.h>
#include <kdbmacros.h>
#include <kdbnotificationinternal.h>
#include <kdbplugin.h>
#include <kdbtypes.h>
#ifdef ELEKTRA_ENABLE_OPTIMIZATIONS
#include <kdbopmphm.h>
#include <kdbopmphmpredictor.h>
#endif

#include <limits.h>

/** The minimal allocation size of a keyset inclusive
	NULL byte. ksGetAlloc() will return one less because
	it says how much can actually be stored.*/
#define KEYSET_SIZE 16

/** Trie optimization */
#define APPROXIMATE_NR_OF_BACKENDS 16

/** The maximum value of unsigned char+1, needed
 *  for iteration over trie children/values:
 *
 *  for (i=0; i<KDB_MAX_UCHAR; ++i)
 * */
#define KDB_MAX_UCHAR (UCHAR_MAX + 1)


/**The maximum of how many characters an integer
  needs as decimal number.*/
#define MAX_LEN_INT 31

/**Backend mounting information.
 *
 * This key directory tells you where each backend is mounted
 * to which mountpoint. */
#define KDB_SYSTEM_ELEKTRA "system:/elektra"

/** All keys below this are used for cache metadata in the global keyset */
#define KDB_CACHE_PREFIX "system:/elektra/cache"

#define ELEKTRA_RECORD_CONFIG_KEY "/elektra/record/config"
#define ELEKTRA_RECORD_CONFIG_ACTIVE_KEY "/elektra/record/config/active"
#define ELEKTRA_RECORD_SESSION_KEY "/elektra/record/session"
#define ELEKTRA_RECORD_SESSION_DIFF_ADDED_KEY "/elektra/record/session/diff/added"
#define ELEKTRA_RECORD_SESSION_DIFF_MODIFIED_OLD_KEY "/elektra/record/session/diff/modified/old"
#define ELEKTRA_RECORD_SESSION_DIFF_MODIFIED_NEW_KEY "/elektra/record/session/diff/modified/new"
#define ELEKTRA_RECORD_SESSION_DIFF_REMOVED_KEY "/elektra/record/session/diff/removed"

#ifdef __cplusplus
namespace ckdb
{
extern "C" {

/** Test a bit. @see set_bit(), clear_bit() */
#define test_bit(var, bit) ((static_cast<unsigned long long> (var)) & (static_cast<unsigned long long> (bit)))
/** Set a bit. @see clear_bit() */
#define set_bit(var, bit) ((var) |= (static_cast<unsigned long long> (bit)))
/** Clear a bit. @see set_bit() */
#define clear_bit(var, bit) ((var) &= ~(static_cast<unsigned long long> (bit)))

#else

/** Test a bit. @see set_bit(), clear_bit() */
#define test_bit(var, bit) (((unsigned long long) (var)) & ((unsigned long long) (bit)))
/** Set a bit. @see clear_bit() */
#define set_bit(var, bit) ((var) |= ((unsigned long long) (bit)))
/** Clear a bit. @see set_bit() */
#define clear_bit(var, bit) ((var) &= ~((unsigned long long) (bit)))

#endif


/* These define the type for pointers to all the kdb functions */
typedef int (*kdbOpenPtr) (Plugin *, Key * errorKey);
typedef int (*kdbClosePtr) (Plugin *, Key * errorKey);

typedef int (*kdbInitPtr) (Plugin * handle, KeySet * definition, Key * parentKey);
typedef int (*kdbGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbSetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbErrorPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbCommitPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookGoptsGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookSpecCopyPtr) (Plugin * handle, KeySet * returned, Key * parentKey, bool isKdbGet);
typedef int (*kdbHookSpecRemovePtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookSendNotificationGetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);
typedef int (*kdbHookSendNotificationSetPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef int (*kdbHookRecordLockPtr) (Plugin * handle, Key * parentKey);
typedef int (*kdbHookRecordUnlockPtr) (Plugin * handle, Key * parentKey);
typedef int (*kdbHookRecordPtr) (Plugin * handle, KeySet * returned, Key * parentKey);

typedef Plugin * (*OpenMapper) (const char *, const char *, KeySet *);
typedef int (*CloseMapper) (Plugin *);

/**
 * The private copy-on-write key data structure.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs.
 *
 */
struct _KeyData
{
	/**
	 * The value, which is a NULL terminated string or binary.
	 * @see keyString(), keyBinary(),
	 * @see keyGetString(), keyGetBinary(),
	 * @see keySetString(), keySetBinary()
	 */
	union
	{
		char * c;
		void * v;
	} data;

	/**
	 * Size of the value, in bytes, including ending NULL.
	 * @see keyGetValueSize()
	 */
	size_t dataSize;

	/**
	 * Reference counter
	 */
	uint16_t refs;

	/**
	 * Is this structure and its data stored in an mmap()ed memory area?
	 */
	bool isInMmap : 1;

	/**
	 * Bitfield reserved for future use.
	 * Decrease size when adding new flags.
	 */
	int : 15;
};

/**
 * The private copy-on-write keyname structure.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs.
 *
 */
struct _KeyName
{
	/**
	 * The canonical (escaped) name of the key.
	 * @see keyGetName(), keySetName()
	 */
	char * key;

	/**
	 * Size of the name, in bytes, including ending NULL.
	 * @see keyGetName(), keyGetNameSize(), keySetName()
	 */
	size_t keySize;

	/**
	 * The unescaped name of the key.
	 * Note: This is NOT a standard null-terminated string.
	 * @see keyGetName(), keySetName()
	 */
	char * ukey;

	/**
	 * Size of the unescaped key name in bytes, including all NULL.
	 * @see keyBaseName(), keyUnescapedName()
	 */
	size_t keyUSize;

	/**
	 * Reference counter
	 */
	uint16_t refs;

	/**
	 * Is this structure and its data stored in an mmap()ed memory area?
	 */
	bool isInMmap : 1;

	/**
	 * Bitfield reserved for future use.
	 * Decrease size when adding new flags.
	 */
	int : 15;
};

// private methods for COW keys
struct _KeyName * keyNameNew (void);
struct _KeyName * keyNameCopy (struct _KeyName * source);
uint16_t keyNameRefInc (struct _KeyName * keyname);
uint16_t keyNameRefDec (struct _KeyName * keyname);
uint16_t keyNameRefDecAndDel (struct _KeyName * keyname);
void keyNameDel (struct _KeyName * keyname);

void keyDetachKeyName (Key * key);

struct _KeyData * keyDataNew (void);
uint16_t keyDataRefInc (struct _KeyData * keydata);
uint16_t keyDataRefDec (struct _KeyData * keydata);
uint16_t keyDataRefDecAndDel (struct _KeyData * keydata);
void keyDataDel (struct _KeyData * keydata);

static inline bool isKeyNameInMmap (const struct _KeyName * keyname)
{
	return keyname->isInMmap;
}

static inline void setKeyNameIsInMmap (struct _KeyName * keyname, bool isInMmap)
{
	keyname->isInMmap = isInMmap;
}

static inline bool isKeyDataInMmap (const struct _KeyData * keydata)
{
	return keydata->isInMmap;
}

static inline void setKeyDataIsInMmap (struct _KeyData * keydata, bool isInMmap)
{
	keydata->isInMmap = isInMmap;
}

/**
 * The private Key struct.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs. Use the @ref key "Key access methods" instead.
 * Only a backend writer needs to have access to the private attributes of the
 * Key object which is defined as:
 * @code
typedef struct _Key Key;
 * @endcode
 *
 * @ingroup backend
 */
struct _Key
{
	/**
	 * Copy-on-write structure for the key data
	 */
	struct _KeyData * keyData;

	/**
	 * Copy-on-write structure for the key name
	 */
	struct _KeyName * keyName;

	/**
	 * All the key's meta information.
	 */
	KeySet * meta;

	/**
	 * Reference counter
	 */
	uint16_t refs;

	/**
	 * Is this structure stored in an mmap()ed memory area?
	 */
	bool isInMmap : 1;

	/**
	 * Read only flag for name.
	 * Key name is read only and not allowed to be changed.
	 * All attempts to change the name will lead to an error.
	 * Needed for metakeys and keys that are in a data structure that depends on name ordering.
	 */
	bool hasReadOnlyName : 1;

	/**
	 * Read only flag for value.
	 * Key value is read only and not allowed  to be changed.
	 * All attempts to change the value will lead to an error.
	 * Needed for metakeys
	 */
	bool hasReadOnlyValue : 1;

	/**
	 * Read only flag for meta.
	 * Key meta is read only and not allowed to be changed.
	 * All attempts to change the value will lead to an error.
	 * Needed for metakeys.
	 */
	bool hasReadOnlyMeta : 1;

	/**
	 * Bitfield reserved for future use.
	 * Decrease size when adding new flags.
	 */
	int : 11;
};

struct _KeySetData
{
	struct _Key ** array; /**<Array which holds the keys */

	size_t size;  /**< Number of keys contained in the KeySet */
	size_t alloc; /**< Allocated size of array */

#ifdef ELEKTRA_ENABLE_OPTIMIZATIONS
	/**
	 * The Order Preserving Minimal Perfect Hash Map.
	 */
	Opmphm * opmphm;
	/**
	 * The Order Preserving Minimal Perfect Hash Map Predictor.
	 */
	OpmphmPredictor * opmphmPredictor;
#endif

	uint16_t refs; /**< Reference counter */

	/**
	 * Is this structure and its data stored in an mmap()ed memory area?
	 */
	bool isInMmap : 1;

	/**
	 * Whether opmphm needs to be rebuilt
	 */
	bool isOpmphmInvalid : 1;

	/**
	 * Bitfield reserved for future use.
	 * Decrease size when adding new flags.
	 */
	int : 14;
};

// COW methods for keyset

struct _KeySetData * keySetDataNew (void);
uint16_t keySetDataRefInc (struct _KeySetData * keysetdata);
uint16_t keySetDataRefDec (struct _KeySetData * keysetdata);
uint16_t keySetDataRefDecAndDel (struct _KeySetData * keysetdata);
void keySetDataDel (struct _KeySetData * keysetdata);

void keySetDetachData (KeySet * keyset);

static inline bool isKeySetDataInMmap (const struct _KeySetData * keysetdata)
{
	return keysetdata->isInMmap;
}

static inline void setKeySetDataIsInMmap (struct _KeySetData * keysetdata, bool isInMmap)
{
	keysetdata->isInMmap = isInMmap;
}


/**
 * The private KeySet structure.
 *
 * Its internal private attributes should not be accessed directly by regular
 * programs. Use the @ref keyset "KeySet access methods" instead.
 * Only a backend writer needs to have access to the private attributes of the
 * KeySet object which is defined as:
 * @code
typedef struct _KeySet KeySet;
 * @endcode
 *
 * @ingroup backend
 */
struct _KeySet
{
	/**
	 * Copy-on-write data
	 */
	struct _KeySetData * data;

	struct _Key * cursor; /**< Internal cursor */
	size_t current;	      /**< Current position of cursor */

	uint16_t refs; /**< Reference counter */

	/**
	 * Is this structure stored in an mmap()ed memory area?
	 */
	bool isInMmap : 1;

	/**
	 * KeySet need sync.
	 * If keys were popped from the Keyset this flag will be set,
	 * so that the backend will sync the keys to database.
	 */
	bool needsSync : 1;

	/**
	 * Bitfield reserved for future use.
	 * Decrease size when adding new flags.
	 */
	int : 14;
};

typedef struct _SendNotificationHook
{
	struct _Plugin * plugin;
	struct _SendNotificationHook * next;

	/**
	 * Optional, may be NULL
	 */
	kdbHookSendNotificationGetPtr get;

	/**
	 * Optional, may be NULL
	 */
	kdbHookSendNotificationSetPtr set;
} SendNotificationHook;

struct _ElektraDiff
{
	Key * parentKey;
	KeySet * addedKeys;
	/**
	 * stores the old versions of modified keys
	 */
	KeySet * modifiedKeys;
	KeySet * removedKeys;

	/**
	 * optional; stores the new version of modified keys
	 */
	KeySet * modifiedNewKeys;

	uint16_t refs;
};

struct _ElektraDiff * elektraDiffNew (KeySet * addedKeys, KeySet * removedKeys, KeySet * modifiedKeys, KeySet * modifiedNewKeys,
				      Key * parentKey);
void elektraDiffAppend (struct _ElektraDiff * target, const struct _ElektraDiff * source, Key * parentKey);
KeySet * elektraDiffGetModifiedNewKeys (const struct _ElektraDiff * ksd);

struct _ChangeTrackingContext
{
	KeySet * oldKeys;
};

struct _ChangeTrackingContext * elektraChangeTrackingCreateContextForTesting (KeySet * oldKeys);
void elektraChangeTrackingContextDel (struct _ChangeTrackingContext * context);

/**
 * The access point to the key database.
 *
 * The structure which holds all information about loaded backends.
 *
 * Its internal private attributes should not be accessed directly.
 *
 * See kdb mount tool to mount new backends.
 *
 * KDB object is defined as:
 * @code
typedef struct _KDB KDB;
 * @endcode
 *
 * @see kdbOpen() and kdbClose() for external use
 * @ingroup backend
 */

struct _KDB
{
	KeySet * modules; /*!< A list of all modules loaded at the moment.*/

	KeySet * global; /*!< This keyset can be used by plugins to pass data through
			the KDB and communicate with other plugins. Plugins shall clean
			up their parts of the global keyset, which they do not need any more.*/

	KeySet * backends;

	struct
	{
		struct
		{
			struct _Plugin * plugin;
			kdbHookGoptsGetPtr get;
		} gopts;

		struct
		{
			struct _Plugin * plugin;
			kdbHookSpecCopyPtr copy;
			kdbHookSpecRemovePtr remove;
		} spec;

		struct _SendNotificationHook * sendNotification;

		struct
		{
			struct _Plugin * plugin;
			kdbHookRecordLockPtr lock;
			kdbHookRecordUnlockPtr unlock;
			kdbHookRecordPtr record;
		} record;
	} hooks;

	KeySet * allKeys;

	struct _ChangeTrackingContext changeTrackingContext;
};

/**
 * Holds all information related to a plugin.
 *
 * Since Elektra 0.8 a Backend consists of many plugins.
 *
 * A plugin should be reusable and only implement a single concern.
 * Plugins which are supplied with Elektra are located below src/plugins.
 * It is no problem that plugins are developed external too.
 *
 * @ingroup backend
 */
struct _Plugin
{
	KeySet * config; /*!< This keyset contains configuration for the plugin.
	 Direct below system:/ there is the configuration supplied for the backend.
	 Direct below user:/ there is the configuration supplied just for the
	 plugin, which should be of course preferred to the backend configuration.
	 The keys inside contain information like /path which path should be used
	 to write configuration to or /host to which host packets should be send.
	 @see elektraPluginGetConfig() */

	kdbOpenPtr kdbOpen;   /*!< The pointer to kdbOpen_template() of the backend. */
	kdbClosePtr kdbClose; /*!< The pointer to kdbClose_template() of the backend. */

	kdbInitPtr kdbInit;	/*!< The pointer to kdbInit_template() of the backend. */
	kdbGetPtr kdbGet;	/*!< The pointer to kdbGet_template() of the backend. */
	kdbSetPtr kdbSet;	/*!< The pointer to kdbSet_template() of the backend. */
	kdbErrorPtr kdbError;	/*!< The pointer to kdbError_template() of the backend. */
	kdbCommitPtr kdbCommit; /*!< The pointer to kdbCommit_template() of the backend. */

	const char * name; /*!< The name of the module responsible for that plugin. */

	size_t refcounter; /*!< This refcounter shows how often the plugin
	   is used.  Not shared plugins have 1 in it */

	void * data; /*!< This handle can be used for a plugin to store
	 any data its want to. */

	KeySet * global; /*!< This keyset can be used by plugins to pass data through
			the KDB and communicate with other plugins. Plugins shall clean
			up their parts of the global keyset, which they do not need any more.*/

	KeySet * modules; /*!< A list of all currently loaded modules.*/
};

/**
 * Holds all data for one backend.
 *
 * This struct is used for the key values in @ref _KDB.backends
 *
 * @ingroup backend
 */
typedef struct _BackendData
{
	struct _Plugin * backend;    /*!< the backend plugin for this backend */
	struct _KeySet * keys;	     /*!< holds the keys for this backend, assigned by backendsDivide() */
	struct _KeySet * plugins;    /*!< Holds all the plugins of this backend.
	    The key names are all `system:/<ref>` where `<ref>` is the same as in
	    `system:/elektra/mountpoints/<mp>/plugins/<ref>` */
	struct _KeySet * definition; /*!< Holds all the mountpoint definition of this backend.
	 This is a copy of `system:/elektra/mountpoints/<mp>/defintion` moved to `system:/` */
	size_t getSize;		     /*!< the size of @ref _BackendData.keys at the end of kdbGet()
		      More precisely this is set by backendsMerge() to the size of @ref _BackendData.keys */
	bool initialized;	     /*!< whether or not the init function of this backend has been called */
} BackendData;

// clang-format on

/***************************************
 *
 * Not exported functions, for internal use only
 *
 **************************************/

/* Backends handling */
Key * backendsFindParent (KeySet * backends, const Key * key);
KeySet * backendsForParentKey (KeySet * backends, Key * parentKey);
bool backendsDivide (KeySet * backends, const KeySet * ks);
void backendsMerge (KeySet * backends, KeySet * ks);

/* Mountpoint parsing */
// visible for testing
KeySet * elektraMountpointsParse (KeySet * elektraKs, KeySet * modules, KeySet * global, Key * errorKey);

/* Plugin handling */
Plugin * elektraPluginOpen (const char * backendname, KeySet * modules, KeySet * config, Key * errorKey);
int elektraPluginClose (Plugin * handle, Key * errorKey);
size_t elektraPluginGetFunction (Plugin * plugin, const char * name);

/* Hooks handling */
int initHooks (KDB * kdb, const KeySet * config, KeySet * modules, const KeySet * contract, Key * errorKey);
void freeHooks (KDB * kdb, Key * errorKey);
Plugin * elektraFindInternalNotificationPlugin (KDB * kdb);


/*Private helper for key*/
ssize_t keySetRaw (Key * key, const void * newBinary, size_t dataSize);
void keyInit (Key * key);
int keyReplacePrefix (Key * key, const Key * oldPrefix, const Key * newPrefix);

static inline KeySet * keyMetaNoAlloc (const Key * key)
{
	if (!key) return NULL;
	return key->meta;
}


/*Private helper for keyset*/
int ksInit (KeySet * ks);
int ksClose (KeySet * ks);

int ksResize (KeySet * ks, size_t size);
size_t ksGetAlloc (const KeySet * ks);
KeySet * ksDeepDup (const KeySet * source);

Key * elektraKsPopAtCursor (KeySet * ks, elektraCursor pos);

KeySet * ksRenameKeys (KeySet * config, const char * name);

ssize_t ksRename (KeySet * ks, const Key * root, const Key * newRoot);

elektraCursor ksFindHierarchy (const KeySet * ks, const Key * root, elektraCursor * end);
KeySet * ksBelow (const KeySet * ks, const Key * root);

ssize_t ksSubtract (KeySet * total, const KeySet * sub);

/*Used for internal memcpy/memmove*/
ssize_t elektraMemcpy (Key ** array1, Key ** array2, size_t size);
ssize_t elektraMemmove (Key ** array1, Key ** array2, size_t size);

/*Internally used for array handling*/
int elektraReadArrayNumber (const char * baseName, kdb_long_long_t * oldIndex);

/* Conveniences Methods for Making Tests */

int keyIsSpec (const Key * key);
int keyIsProc (const Key * key);
int keyIsDir (const Key * key);
int keyIsSystem (const Key * key);
int keyIsUser (const Key * key);

elektraNamespace elektraReadNamespace (const char * namespaceStr, size_t len);

bool elektraKeyNameValidate (const char * name, bool isComplete);
void elektraKeyNameCanonicalize (const char * name, char ** canonicalName, size_t * canonicalSizePtr, size_t offset, size_t * usizePtr);
void elektraKeyNameUnescape (const char * name, char * unescapedName);
size_t elektraKeyNameEscapePart (const char * part, char ** escapedPart);

// TODO (kodebaach) [Q]: make public?
int elektraIsArrayPart (const char * namePart);

int elektraBootstrapPathContract (KeySet * contract, const char * prefix);
#ifdef __cplusplus
}
}

#define KDB ckdb::KDB
#define Key ckdb::Key
#define KeySet ckdb::KeySet
extern "C" {
#endif

struct _Elektra
{
	KDB * kdb;
	Key * parentKey;
	KeySet * config;
	KeySet * defaults;
	Key * lookupKey;
	ElektraErrorHandler fatalErrorHandler;
	char * resolvedReference;
	size_t parentKeyLength;
};

struct _ElektraError
{
	char * code;
	char * codeFromKey;
	char * description;
	char * module;
	char * file;
	kdb_long_t line;
	kdb_long_t warningCount;
	kdb_long_t warningAlloc;
	struct _ElektraError ** warnings;
	Key * errorKey;
};

/* high-level API */
void elektraSaveKey (Elektra * elektra, Key * key, ElektraError ** error);
void elektraSetLookupKey (Elektra * elektra, const char * name);
void elektraSetArrayLookupKey (Elektra * elektra, const char * name, kdb_long_long_t index);

ElektraError * elektraErrorCreate (const char * code, const char * description, const char * module, const char * file, kdb_long_t line);
void elektraErrorAddWarning (ElektraError * error, ElektraError * warning);
ElektraError * elektraErrorFromKey (Key * key);

ElektraError * elektraErrorKeyNotFound (const char * keyname);
ElektraError * elektraErrorWrongType (const char * keyname, KDBType expectedType, KDBType actualType);
ElektraError * elektraErrorNullError (const char * function);
ElektraError * elektraErrorEnsureFailed (const char * reason);

#ifdef __cplusplus
}
#undef Key
#undef KeySet
#undef KDB
#endif

#endif /* KDBPRIVATE_H */

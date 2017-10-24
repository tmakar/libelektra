#!/bin/sh

@INCLUDE_COMMON@

set -f

FILE=$1
Mountpoint=
DBFile=
Storage=
MountArgs=
DiffType=File
OutFile=$(mktempfile_elektra)

RETCMP=
ERRORSCMP=
WARNINGSCMP=
STDOUTCMP=
STDOUTRECMP=
STDOUTGLOBCMP=
STDERRCMP=
DIFFCMP=

BACKUP=0
TMPFILE=$(mktempfile_elektra)

# variables to count up errors and tests
nbError=0
nbTest=0

execute()
{
	proto="$*"

	if [ -z "$Mountpoint" ];
	then
		echo 'Error: no mountpoint specified in script'
		exit 1
	fi

	if [ -z "$DBFile" ];
	then
		DBFile=$("$KDBCOMMAND" file "$Mountpoint" 2>/dev/null)
	fi

	if [ "$BACKUP" -eq '1' ];
	then
		if ! "$KDBCOMMAND" export "$Mountpoint" dump > "$TMPFILE" 2>/dev/null;
		then
			echo "ERROR: Failed to backup $Mountpoint\nStopping testcase."
			exit 1
		fi
		BACKUP=0
		"$KDBCOMMAND" rm -r "$Mountpoint" 2>/dev/null
	fi

	[ -z "$Storage" ] && Storage="dump"
	command=$(echo "$proto" | sed -e "s~\$Mountpoint~${Mountpoint}~g" \
	                              -e "s~\$File~${DBFile}~g"           \
	                              -e "s~\$Storage~${Storage}~g"       \
	                              -e "s~\$MountArgs~${MountArgs}~g")

	case "$DiffType" in
	File)
		rm -f "${DBFile}.1"
		cp "${DBFile}" "${DBFile}.1" 2>/dev/null
		;;
	Ini)
		"$KDBCOMMAND" export "$Mountpoint" ini > ./previousState 2>/dev/null
		;;
	Dump)
		"$KDBCOMMAND" export "$Mountpoint" dump > ./previousState 2>/dev/null
		;;
	esac

	echo "$command"

	printf 'CMD: %s\n' "$command" >> "$OutFile"

	sh -c -f "$command" 2>stderr 1>stdout

	RETVAL="$?"

	printf 'RET: %s\n' "$RETVAL" >> "$OutFile"

	if [ ! -z "$RETCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! echo "$RETVAL" | grep -Ewq $RETCMP;
		then
			printf 'Return value “%s” does not match “%s”\n' "$RETVAL" "$RETCMP"
			printf '=== FAILED return value does not match expected pattern %s\n' "$RETCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi

	DIFF=
	case "$DiffType" in
	File)
		DIFF=$(diff -N --text "${DBFile}" "${DBFile}.1" 2>/dev/null)
		;;
	Ini)
		"$KDBCOMMAND" export $Mountpoint ini > ./newState 2>/dev/null
		DIFF=$(diff -N --text ./previousState ./newState 2>/dev/null)
		rm -f ./newState ./previousState
		;;
	Dump)
		"$KDBCOMMAND" export $Mountpoint dump > ./newState 2>/dev/null
		DIFF=$(diff -N --text ./previousState ./newState 2>/dev/null)
		rm -f ./newState ./previousState
		;;
	esac



	STDERR=$(cat ./stderr)


	printf 'STDERR: %s\n' "$STDERR" >> "$OutFile"
	if [ ! -z "$STDERRCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! echo "$STDERR" | replace_newline_return | grep -Eq --text "$STDERRCMP";
		then
			printf '\nERROR - STDERR:\n“%s”\ndoes not match “%s”\n\n' "$STDERR" "$STDERRCMP"
			printf '=== FAILED stderr does not match expected pattern %s\n' "$STDERRCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi



	STDOUT=$(cat ./stdout)

	printf '%s\n' "STDOUT: $STDOUT" >> "$OutFile"
	if [ ! -z "$STDOUTCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! printf '%s' "$STDOUT" | replace_newline_return | grep -Eq --text "^${STDOUTCMP}$";
		then
			printf '\nERROR - STDOUT:\n“%s”\ndoes not match “%s”\n\n' "$STDOUT" "$STDOUTCMP"
			printf '=== FAILED stdout does not match expected pattern %s\n' "$STDOUTCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi
	if [ ! -z "$STDOUTRECMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if !  printf '%s' "$STDOUT" | replace_newline_return | grep -Eq --text "$STDOUTRECMP";
		then
			printf '\nERROR - STDOUT:\n“%s”\ndoes not match “%s”\n\n' "$STDOUT" "$STDOUTRECMP"
			printf '=== FAILED stdout does not match expected pattern %s\n' "$STDOUTRECMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi

	WARNINGS=$(echo "$STDERR" | sed -nE  's/Warning number: (\d*)/\1/p' | tr '\n' ',')

	printf 'WARNINGS: %s\n' "$WARNINGS" >> "$OutFile"
	if [ ! -z "$WARNINGSCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! echo "$WARNINGS" | replace_newline_return | grep -Eq --text "$WARNINGSCMP";
		then
			printf '\nERROR - WARNINGS:\n“%s”\ndoes not match “%s”\n\n' "$WARNINGS" "$WARNINGSCMP"
			printf '=== FAILED Warnings do not match expected pattern %s\n' "$WARNINGSCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi




	ERRORS=$(echo "$STDERR" | sed -nE  's/error \(\#(\d*)/\1/p' | tr '\n' ',')


	printf 'ERRORS: %s\n' "$ERRORS" >> "$OutFile"
	if [ ! -z "$ERRORSCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! echo "$ERRORS" | replace_newline_return | grep -Eq --text "$ERRORSCMP";
		then
			printf '\nERROR - ERRORS:\n“%s”\ndoes not match “%s”\n\n' "$ERRORS" "$ERRORSCMP"
			printf '=== FAILED Errors do not match expected pattern %s\n' "$ERRORSCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi



	printf '%s\n' "DIFF: $DIFF" >> "$OutFile"
	if [ ! -z "$DIFFCMP" ];
	then
		nbTest=$(( nbTest + 1 ))
		if ! echo "$DIFF" | replace_newline_return | grep -Eq --text "$DIFFCMP";
		then
			printf '\nERROR - Changes to %s:\n“%s”\ndo not match “%s”\n\n' "$DBFile" "$DIFFCMP" "$DIFF"
			printf '=== FAILED changes to database file (%s) do not match %s\n' "$DBFile" "$DIFFCMP" >> "$OutFile"
			nbError=$(( nbError + 1 ))
		fi
	fi


	echo >> "$OutFile"
}

run_script()
{
	while read -r line;
	do
	OP=
	ARG=
	cmd=$(printf '%s' "$line"|cut -d ' ' -f1)
	case "$cmd" in
	Mountpoint:)
		Mountpoint=$(echo "$line"|cut -d ' ' -f2)
		;;
	File:)
		DBFile=$(echo "$line"|cut -d ' ' -f2)
		if [ "$DBFile" = "File:" ] || [ -z "$DBFile" ]; then
			DBFile=$(mktempfile_elektra)
		fi
		;;
	Storage:)
		Storage=$(echo "$line"|cut -d ' ' -f2)
		;;
	MountArgs:)
		MountArgs=$(echo "$line"|cut -d ' ' -f2-)
		;;
	DiffType:)
		DiffType=$(echo "$line"|cut -d ' ' -f2)
		;;
	RET:)
		RETCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	ERRORS:)
		ERRORSCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	WARNINGS:)
		WARNINGSCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	STDOUT:)
		STDOUTCMP=$(printf '%s' "$line"|cut -d ' ' -f2-)
		;;
	STDOUT-REGEX:)
		STDOUTRECMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	STDOUT-GLOB:)
		STDOUTGLOBCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	STDERR:)
		STDERRCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	DIFF:)
		DIFFCMP=$(echo "$line"|cut -d ' ' -f2-)
		;;
	\<)
		OP="$cmd"
		ARG=$(printf '%s' "$line"|cut -d ' ' -f2-)
		;;
	esac
	if [ "$OP" = "<" ];
	then
		execute "$ARG"
		RETCMP=
		ERRORSCMP=
		WARNINGSCMP=
		STDOUTCMP=
		STDOUTRECMP=
		STDOUTGLOBCMP=
		STDERRCMP=
		DIFFCMP=
	fi
	done < "$FILE"
}

rm -f ./stdout ./stderr

if [ "$#" -lt '1' ] || [ "$#" -gt '2' ];
then
	echo 'Usage: ./shell_recorder inputscript [protocol to compare]'
	rm "$OutFile"
	exit 0
fi

BACKUP=1

echo "protocol file: $OutFile"

run_script

"$KDBCOMMAND" rm -r "$Mountpoint" 2>/dev/null
"$KDBCOMMAND" import "$Mountpoint" dump 2>/dev/null < "$TMPFILE"
rm -rf "${DBFile}.1"

EVAL=0

if [ "$#" -eq '1' ];
then
	printf 'shell_recorder %s RESULTS: %s test(s) done' "$1" "$nbTest"
	echo " $nbError error(s)."
	EVAL=$nbError
fi

if [ "$#" -eq '2' ];
then
	RESULT=$(diff -N --text "$2" "$OutFile" 2>/dev/null)
	if [ "$?" -ne '0' ];
	then
		printf '=======================================\nReplay test failed, protocols differ\n'
		echo "$RESULT"
		printf '\n\n\n'
		EVAL=1
	else
		printf '=======================================\nReplay test succeeded\n'
	fi
fi

# this should be in temporary files, and/or in a trap exit
rm -f ./stdout ./stderr

rm "${TMPFILE}"
exit "$EVAL"

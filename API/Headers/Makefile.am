## Process this file with automake to produce Makefile.in

SUBDIRS = private

bufrIncludedir=@includedir@/@PACKAGE@@VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_MICRO@

INCLUDE_FILES = $(wildcard *.h)

# Default configuration without enabling anything
#INCLUDE_FILES = ${FRAMEWORK_INCLUDES}

bufrInclude_HEADERS = ${INCLUDE_FILES}

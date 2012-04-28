MACRO(CreateSourceGroups Group Root FileList)
    FOREACH(File ${FileList})
        IF (${Root})
            STRING (REGEX REPLACE "${Root}" "" RelativePath "${File}")
        ELSE (${Root})
            SET (RelativePath "${File}")
        ENDIF (${Root})
        STRING (REGEX REPLACE "[\\\\/][^\\\\/]*$" "" RelativePath "${RelativePath}")
        STRING (REGEX REPLACE "^[\\\\/]" "" RelativePath "${RelativePath}")
        STRING (REGEX REPLACE "/" "\\\\\\\\" RelativePath "${RelativePath}")
        SOURCE_GROUP ( "${Group}\\${RelativePath}" FILES ${File} )
    ENDFOREACH(File)
ENDMACRO(CreateSourceGroups)

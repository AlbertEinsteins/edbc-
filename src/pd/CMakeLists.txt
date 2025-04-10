add_library(
    edb_pd
    "pd.cpp"
)

set(ALL_OBJECT_FILES
    ${ALL_OBJECT_FILES} $<TARGET_OBJECTS:edb_pd>
    PARENT_SCOPE)
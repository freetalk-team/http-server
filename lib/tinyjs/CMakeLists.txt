

add_library(tinyjs STATIC
	TinyJS.cpp 
	TinyJS_Functions.cpp 
	TinyJS_MathFunctions.cpp 
	dregex.cpp 
	# cbl_String.cpp
	String.cpp
)

# set_property(TARGET tinyjs PROPERTY CXX_STANDARD 23)

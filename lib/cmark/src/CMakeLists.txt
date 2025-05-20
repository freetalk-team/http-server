
configure_file(cmark_version.h.in
	${CMAKE_CURRENT_BINARY_DIR}/cmark_version.h)
configure_file(libcmark.pc.in
	${CMAKE_CURRENT_BINARY_DIR}/libcmark.pc
	@ONLY)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_library(cmark STATIC
	blocks.c
	buffer.c
	cmark.c
	cmark_ctype.c
	commonmark.c
	houdini_href_e.c
	houdini_html_e.c
	houdini_html_u.c
	html.c
	inlines.c
	iterator.c
	latex.c
	man.c
	node.c
	references.c
	render.c
	scanners.c
	scanners.re
	utf8.c
	xml.c)

cmark_add_compile_options(cmark)

target_compile_definitions(cmark PUBLIC CMARK_STATIC_DEFINE)

generate_export_header(cmark BASE_NAME cmark)

# Set up include directories
target_include_directories(cmark
		PUBLIC
				${CMAKE_CURRENT_SOURCE_DIR}   # Where cmark.h lives
				${CMAKE_CURRENT_BINARY_DIR}   # Where cmark_export.h is generated
)

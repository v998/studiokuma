#ifndef MIM_LIBJSON_H
#define MIM_LIBJSON_H

#define IsJSONServiceAvailable() (ServiceExists(MS_JSON_FREE))

// This function removes the memory allocated by various functions that return strings,
// such as MS_JSON_AS_*, json_write...
// wParam=(WPARAM)(LPVOID)pVoid
// lParam=0
// Return value ALWAYS 0
#define MS_JSON_FREE "JSON/Free"

// Delete specified node and all its children
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Return value ALWAYS 0
#define MS_JSON_DELETE "JSON/Delete"

// <!-- These functions are only available if JSON_MEMORY_MANAGE is defined on building
/*
// This function removes all of the memory that libjson has allocated for strings and binary.
// This allows for bulk frees and garbage collection.
// wParam=lParam=0
// Return value ALWAYS 0
#define MS_JSON_FREE_ALL "JSON/FreeAll"

// Destructs the JSONNode object and cleans itself up.
// wParam=lParam=0
// Return value ALWAYS 0
#define MS_JSON_DELETE_ALL "JSON/DeleteAll"
*/
// -->

// This function parses JSON text and returns you a HANDLE which is the root node of the text 
// that you just passed it.  If bad JSON is sent to this method it may return NULL.
// wParam=(WPARAM)(LPCSTR)szJSON
// lParam=0
// Returns a HANDLE on success, NULL on failure
// * Remember to free the handle with MS_JSON_DELETE
#define MS_JSON_PARSE "JSON/Parse"

// This function removes anything that the JSON standard defines as white space, 
// including extra tabs, spaces, formatting, and comments.  This makes this function useful 
// for compressing json that needs to be stored or sent over a network.
// wParam=(WPARAM)(LPCSTR)szJSON
// lParam=0
// Returns a newly allocated valid JSON that is free of all white space
// * Remember to free the string with MS_JSON_FREE
#define MS_JSON_STRIP_WHITE_SPACE "JSON/StripWhiteSpace"


// <!-- These functions are only available if JSON_VALIDATE is defined on building
/*
// This function validates the text by parsing it completely and looking for anything that is 
// malformed.  If bad JSON is sent to this method it will throw a std::invalid_argument exception, 
// otherwise it returns the root node of the text.
// wParam=(WPARAM)(LPCSTR)szJSON
// lParam=0
// Returns a HANDLE on success
// * Remember to free the handle with MS_JSON_DELETE
#define MS_JSON_VALIDATE "JSON/Validate"
*/
// -->

// This function creates a string node with the name and value specified.  
// wParam=(WPARAM)(LPCSTR)szName
// lParam=(LPARAM)(LPCSTR)szValue
// Returns a newly allocated node
// You must json_delete the resulting node or attach it to something as a child.
#define MS_JSON_NEW_STRING "JSON/NewA"

// This function creates a integer node with the name and value specified.  
// wParam=(WPARAM)(LPCSTR)szName
// lParam=(LPARAM)(long)lValue
// Returns a newly allocated node
// You must json_delete the resulting node or attach it to something as a child.
// Warning: The value IS SIGNED, for unsigned use MS_JSON_NEW_FLOAT instead.
#define MS_JSON_NEW_INTEGER "JSON/NewI"

// This function creates a floating point node with the name and value specified.  
// wParam=(WPARAM)(LPCSTR)szName
// lParam=(LPARAM)(float)fValue
// Returns a newly allocated node
// You must json_delete the resulting node or attach it to something as a child.
#define MS_JSON_NEW_FLOAT "JSON/NewF"

// This function creates a boolean node with the name and value specified.  
// wParam=(WPARAM)(LPCSTR)szName
// lParam=(LPARAM)(int)iValue
// Returns a newly allocated node
// You must json_delete the resulting node or attach it to something as a child.
#define MS_JSON_NEW_BOOLEAN "JSON/NewB"

// This function creates an empty node of the specified type.  
// This would normally be used to start serializing something or writing a configuration file.
// wParam=(WPARAM)(char)cType
// lParam=0
// Returns a newly allocated node
// You must json_delete the resulting node or attach it to something as a child.
#define JSON_NULL '\0'
#define JSON_STRING '\1'
#define JSON_NUMBER '\2'
#define JSON_BOOL '\3'
#define JSON_ARRAY '\4'
#define JSON_NODE '\5'

#define MS_JSON_NEW "JSON/New"

// This function copies a JSONNODE and returns the new copy.  
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns an exact copy of the node parameter
#define MS_JSON_COPY "JSON/Copy"

// Constructs a JSONNODE object, by copying the contents of JSONNODE.  
// This is different from the MS_JSON_COPY because it makes a literal copy, 
// not reference counting. 
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a HANDLE that is a new copy of the original node.
#define MS_JSON_DUPLICATE "JSON/Duplicate"

// Sets the string value of the JSONNODE.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szValue
// Return value ALWAYS 0
#define MS_JSON_SET_STRING "JSON/SetA"

// Sets the integer value of the JSONNODE.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(long)lValue
// Return value ALWAYS 0
// Warning: The value IS SIGNED, for unsigned use MS_JSON_SET_FLOAT instead.
#define MS_JSON_SET_INTEGER "JSON/SetI"

// Sets the floating point value of the JSONNODE.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(float)fValue
// Return value ALWAYS 0
#define MS_JSON_SET_FLOAT "JSON/SetF"

// Sets the boolean value of the JSONNODE.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(int)iValue
// Return value ALWAYS 0
#define MS_JSON_SET_BOOLEAN "JSON/SetB"

// Sets the value of the JSONNODE to the value of the other, 
// usually through fast and simple reference counting.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(HANDLE)hValueNode
// Return value ALWAYS 0
#define MS_JSON_SET_NODE "JSON/SetN"

// Returns the type of the JSONNODE.
// usually through fast and simple reference counting.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns value of JSON_* (See MS_JSON_NEW)
#define MS_JSON_TYPE "JSON/Type"

// Returns the number of children that the node has.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns the number of children
// This should be zero for anything other than JSON_ARRAY or JSON_NODE, 
// but this is only guaranteed with the JSON_SAFE option turned on (by default).
// This is because casting may or may not purge the children.
#define MS_JSON_SIZE "JSON/Size"

// Returns whether or not the node has any children.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns 0 if the node is empty, nonzero if not
// If the node is not of JSON_NODE or JSON_ARRAY it will invariably return true.
#define MS_JSON_EMPTY "JSON/Empty"

// Returns the name of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a newly allocated string.
// If there is no name, then it returns a blank string.
#define MS_JSON_NAME "JSON/Name"

// <!-- These functions are only available if JSON_COMMENTS is defined on building
/*
// Returns the comment attached to the node
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a newly allocated string.
#define MS_JSON_COMMENT "JSON/Comment"

// Sets the comment that will be associated with the JSONNode.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szComment
// Return value ALWAYS 0
#define MS_JSON_SET_COMMENT "JSON/SetComment"
*/
// -->

// Returns the string representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a newly allocated string representation of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: "" or "null" depending on how it was created
// JSON_STRING: The unescaped string value
// JSON_NUMBER: The number in string form (may be in scientific notation)
// JSON_BOOL: "true" or "false"
// JSON_ARRAY: ""
// JSON_NODE: ""
#define MS_JSON_AS_STRING "JSON/AsString"

// Returns the integer representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns the integer representation of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: 0
// JSON_STRING: Undefined
// JSON_NUMBER: Truncated Value
// JSON_BOOL: 1 if true, 0 if false
// JSON_ARRAY: Undefined
// JSON_NODE: Undefined
// Warning: The value IS SIGNED, for unsigned use MS_JSON_AS_FLOAT instead.
// If the value is actually a floating point value, it will continue on ahead 
// and simply truncate the value.  So 15.9 will be returned as 15.
#define MS_JSON_AS_INTEGER "JSON/AsInt"

// Returns the floating point representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns the floating point representation of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: 0.0
// JSON_STRING: Undefined
// JSON_NUMBER: Value
// JSON_BOOL: 1.0 if true, 0.0 if false
// JSON_ARRAY: Undefined
// JSON_NODE: Undefined
#define MS_JSON_AS_FLOAT "JSON/AsFloat"

// Returns the boolean representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns the boolean representation of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: 0.0
// JSON_STRING: Undefined
// JSON_NUMBER: Value == 0.0
// JSON_BOOL: Value
// JSON_ARRAY: Undefined
// JSON_NODE: Undefined
#define MS_JSON_AS_BOOLEAN "JSON/AsBool"

// Returns the boolean representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a HANDLE of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: Empty node
// JSON_STRING: Empty node
// JSON_NUMBER: Empty node
// JSON_BOOL: Empty node
// JSON_ARRAY: Array converted to a node
// JSON_NODE: A copy of the node
// For anything other than node and array, it simply returns an 
// empty node.  If the caller is an array, it will convert it to a node.
// This command creates a new JSONNODE that has to be deleted or attached to a parent.
#define MS_JSON_AS_NODE "JSON/AsNode"

// Returns the array representation of the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a HANDLE of the node.
// The return value may be different depending on the type of the node
// JSON_NULL: Empty node
// JSON_STRING: Empty node
// JSON_NUMBER: Empty node
// JSON_BOOL: Empty node
// JSON_ARRAY: A copy of the array
// JSON_NODE: An array of the children
// For anything other than node and array, it simply returns an 
// empty array.  If the caller is an node, it will convert it to an array by stripping 
// all of the names of each child.
// This command creates a new JSONNODE that has to be deleted or attached to a parent.
#define MS_JSON_AS_ARRAY "JSON/AsArray"

// <!-- These functions are only available if JSON_BINARY is defined on building
/*
// Returns the binary value that was part of this node. 
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPDWORD)pdwSize
// Returns a Base64 decoded binary data as std::string.
// The return value may be different depending on the type of the node
// JSON_NULL: Undefined
// JSON_STRING: The binary data from the decoded Base64
// JSON_NUMBER: Undefined
// JSON_BOOL: Undefined
// JSON_ARRAY: Undefined
// JSON_NODE: Undefined
// It returns it as a std::string, you can use the data() function to retrieve it in binary form.
// This allows you to use size() to know how large the binary data is.
// pdwSize points to a DWORD to receive how many bytes the data is
#define MS_JSON_AS_BINARY "JSON/AsBinary"
*/
// -->

// Returns JSON text, with no white space or comments.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a JSON text of the node being written.
// Remember to free the return value with MS_JSON_FREE
// Designed to create JSON that is very small, and therefore, faster to send between 
// servers or write to a disk. The flipside is that it's nearly impossible to read by human eyes.
// Only root nodes (JSON_NODE and JSON_ARRAYs) are meant to be written, 
// all others will return a blank string.
#define MS_JSON_WRITE "JSON/Write"

// Returns JSON text that has been indented and prettied up so that it can be easily 
// read and modified by humans.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Returns a JSON text of the node being written.
// Remember to free the return value with MS_JSON_FREE
// Only root nodes (JSON_NODE and JSON_ARRAYs) are meant to be written, 
// all others will return a blank string.
#define MS_JSON_WRITE_FORMATTED "JSON/WriteFormatted"

// Sets the name of the JSONNode.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szName
// Return value ALWAYS 0
#define MS_JSON_SET_NAME "JSON/SetName"

// Clears all children from the node.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Return value ALWAYS 0
#define MS_JSON_CLEAR "JSON/Clear"

// Nulls out the node
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Return value ALWAYS 0
#define MS_JSON_NULLIFY "JSON/Nullify"

// Swaps the contents of two nodes. 
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(WPARAM)(HANDLE)hCode2
// Return value ALWAYS 0
// This is very fast because JSONNODE is just a wrapper around an 
// internal structure, so it simply swaps pointers to those structures.
#define MS_JSON_SWAP "JSON/Swap"

// Merge the contents of nodes
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(WPARAM)(HANDLE)hCode2
// Return value ALWAYS 0
// It's possible that you may end up with multiple copies of the same node, 
// through duplication and such.  To save space, you might want to merge the 
// internal reference counted structure.
#define MS_JSON_MERGE "JSON/Merge"

// Preparse the json
// wParam=(WPARAM)(HANDLE)hNode
// lParam=0
// Return value ALWAYS 0
// libjson's lazy parsing makes parsing JSON that is not entirely used very fast, 
// but sometimes you want to parse it all at once, making the next reads a little faster
#define MS_JSON_PREPARSE "JSON/Preparse"

// json_set_binary not reflected as it requires 3 parameters
/*
#ifdef JSON_BINARY
 LIBJSON_DLL void json_set_binary(JSONNODE * node, const void * data, unsigned long length);
#endif
 */

// Will change the node to a different type and do any conversions necessary.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(char)cType
// Return value ALWAYS 0
// Value of cType refer to MS_JSON_NEW
#define MS_JSON_CAST "JSON/Cast"

// This function reserves children space, this makes the program faster and use less memory 
// as it doesn't have to keep allocating new memory when it runs out.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(int)iSize
// Return value ALWAYS 0
// Value of cType refer to MS_JSON_NEW
#define MS_JSON_RESERVE "JSON/Reserve"

// This will give you a reference to a child node at a specific location.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(int)iPos
// Returns the HANDLE to the iPos' node of hNode
// This is a safe function and will return zero if you go out of bounds.
// The returned value is still a child, so do not try and delete the results.
#define MS_JSON_AT "JSON/At"

// This will give you a reference to a child node by its name.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szName
// Returns the HANDLE of the child named szName
// This is a safe function and will return zero if that child does not exist.
// The returned value is still a child, so do not try and delete the results.
#define MS_JSON_GET "JSON/Get"

// <!-- These functions are only available if JSON_CASE_INSENSITIVE_FUNCTIONS is defined on building
/*
// This will give you a reference to a child node by its name.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szName
// Returns the HANDLE of the child named szName
// This is a safe function and will return zero if that child does not exist.
// The returned value is still a child, so do not try and delete the results.
#define MS_JSON_GET_NOCASE "JSON/GetNocase"

// This will give remove a JSONNODE from it's parent and return it to you.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szName
// Returns the HANDLE of the child named szName
// Because it's removed from the parent, you must delete it yourself.
#define MS_JSON_POP_BACK_NOCASE "JSON/PopBackNocase"
*/
// -->

// This function pushes a new child node on the back of the child list.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(HANDLE)hNode2
// Return value ALWAYS 0
// This method copies the child, so altering the parameter later will not affect the one in the children.  
// The child is then managed, so do not try and delete it later.
#define MS_JSON_PUSH_BACK "JSON/PushBack"

// This will give remove a JSONNODE from it's parent and return it to you.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(int)iPos
// Returns the HANDLE of the child named szName
// Because it's removed from the parent, you must delete it yourself.
#define MS_JSON_POP_BACK_AT "JSON/PopBackAt"

// This will give remove a JSONNODE from it's parent and return it to you.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(LPCSTR)szName
// Returns the HANDLE of the child named szName
// Because it's removed from the parent, you must delete it yourself.
#define MS_JSON_POP_BACK "JSON/PopBack"

// functions with JSON_ITERATORS not reflected
		  #ifdef JSON_ITERATORS
			 LIBJSON_DLL JSONNODE_ITERATOR json_find(JSONNODE * node, const json_char * name);
			 #ifdef JSON_CASE_INSENSITIVE_FUNCTIONS
				LIBJSON_DLL JSONNODE_ITERATOR json_find_nocase(JSONNODE * node, const json_char * name);
			 #endif
			 LIBJSON_DLL JSONNODE_ITERATOR json_erase(JSONNODE * node, JSONNODE_ITERATOR it);
			 LIBJSON_DLL JSONNODE_ITERATOR json_erase_multi(JSONNODE * node, JSONNODE_ITERATOR start, JSONNODE_ITERATOR end);
			 LIBJSON_DLL JSONNODE_ITERATOR json_insert(JSONNODE * node, JSONNODE_ITERATOR it, JSONNODE * node2);
			 LIBJSON_DLL JSONNODE_ITERATOR json_insert_multi(JSONNODE * node, JSONNODE_ITERATOR it, JSONNODE_ITERATOR start, JSONNODE_ITERATOR end);
	 
			 //iterator functions
			 LIBJSON_DLL JSONNODE_ITERATOR json_begin(JSONNODE * node);
			 LIBJSON_DLL JSONNODE_ITERATOR json_end(JSONNODE * node);
		  #endif

// Checks if the value held within the nodes are equal.
// wParam=(WPARAM)(HANDLE)hNode
// lParam=(LPARAM)(HANDLE)hNode2
// Returns 0 if not equal, nonzero otherwise
// This ignores things like comments, but for JSON_NODE and JSON_ARRAYs, 
// this is a deep comparison, checking each child too.
#define MS_JSON_EQUAL "JSON/Equal"

#endif // MIM_LIBJSON_H

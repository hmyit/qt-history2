// Generated by src/opengl/util/./glsl_to_include.sh from ellipse.glsl
"!!ARBfp1.0"
"PARAM c[2] = { program.local[0],"
"{ 1 } };"
"TEMP R0;"
"MUL R0.xy, fragment.texcoord[0], fragment.texcoord[0];"
"ADD R0.x, R0, R0.y;"
"SLT R0.x, c[1], R0;"
"MOV result.color, c[0];"
"KIL -R0.x;"
"END"
; // Generated by src/opengl/util/./glsl_to_include.sh from ellipse.glsl

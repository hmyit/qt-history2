// Generated by src/opengl/util/./glsl_to_include.sh from ellipse_aa_copy.glsl
"!!ARBfp1.0"
"PARAM c[3] = { program.local[0],"
"{ 2, 1, -0.55000001, 0.90909088 },"
"{ 3 } };"
"TEMP R0;"
"TEMP R1;"
"MUL R0.xy, fragment.texcoord[0].zwzw, fragment.texcoord[0].zwzw;"
"RCP R0.y, R0.y;"
"RCP R0.x, R0.x;"
"MUL R0.zw, fragment.texcoord[0].xyxy, R0.xyxy;"
"MUL R0.zw, R0, c[1].x;"
"MUL R0.zw, R0, R0;"
"RCP R0.y, fragment.texcoord[0].w;"
"RCP R0.x, fragment.texcoord[0].z;"
"MUL R0.xy, fragment.texcoord[0], R0;"
"MUL R0.xy, R0, R0;"
"ADD R0.x, R0, R0.y;"
"ADD R0.y, R0.z, R0.w;"
"RSQ R0.y, R0.y;"
"ADD R0.x, -R0, c[1].y;"
"MAD R0.x, R0.y, R0, -c[1].z;"
"MUL_SAT R1.x, R0, c[1].w;"
"MUL R1.y, -R1.x, c[1].x;"
"MUL R0.xy, fragment.position, c[0];"
"TEX R0, R0, texture[0], 2D;"
"ADD R1.y, R1, c[2].x;"
"MUL R1.x, R1, R1;"
"MUL R1.x, R1, R1.y;"
"MUL result.color, R1.x, R0;"
"END"
; // Generated by src/opengl/util/./glsl_to_include.sh from ellipse_aa_copy.glsl

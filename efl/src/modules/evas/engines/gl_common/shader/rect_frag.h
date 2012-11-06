"#ifdef GL_ES\n"
"#ifdef GL_FRAGMENT_PRECISION_HIGH\n"
"precision highp float;\n"
"#else\n"
"precision mediump float;\n"
"#endif\n"
"#endif\n"
"uniform sampler2D tex;\n"
"varying vec4 col;\n"
"void main()\n"
"{\n"
"   gl_FragColor = col;\n"
"}\n"
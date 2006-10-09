float ellipse_aa()
{
    vec2 st = gl_TexCoord[0].st;

    float Fxy = 1 - dot(st, st);
    vec2 gradFxy = vec2(ddx(Fxy), ddy(Fxy));
    float g = inversesqrt(dot(gradFxy, gradFxy));

    return smoothstep(-0.5, 0.5, Fxy * g);
}

uniform vec4 solid_color;

void main()
{
    gl_FragColor = solid_color * ellipse_aa();
}

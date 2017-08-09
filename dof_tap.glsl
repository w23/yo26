void main() {
	vec2 uv = X / Z(5);
	float rad = 0.;
	vec2 angle = vec2(0.,1.1);
	mat2 rot = mat2(cos(2.4),sin(2.4),-sin(2.4),cos(2.4));
	vec4 near, far;
	near = far = vec4(.0,.0,.0,.0001);
	float z = P(4,uv).w;
	for (int i = 0; i < 100; i++) {
		vec4 pix = P(4, uv + (rad * angle + .0)/Z(5));

		float A = .04;
		float D = F[5];
		float F = .03 / tan(.7);
		float P = pix.w;
		float coc = abs(A * F / (P - F) * (P / D - 1.)) * Z(1).x / .035;

		if (coc > rad) {
		 	if (pix.w > D)
				far += vec4(pix.xyz, 1.);
			else
				near += vec4(pix.xyz, 1.);
		}

		rad += 1. / (rad + 10.);
		angle *= rot;
	}
	gl_FragData[0] = vec4(near.xyz/near.w, near.w);
	gl_FragData[1] = vec4(far.xyz/far.w, far.w);
}

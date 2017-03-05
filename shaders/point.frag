#version 330


in vec4 fVertex;

out vec4 color;

//uniform vec3 lightpos;
//uniform vec3 campos;
//uniform int mode;
//uniform int coloring;

/// TODO: change depth of each fragment according to the distance to center.
void main() {

    int coloring = 0;
    int mode = 1;


	//if(fVertex.w < 0.1) {
	//	discard;
	//}


	/*
	float transparency = 0.9;

	vec4 lightparams = vec4(0.3, 0.7, 0.2, 10); //<ambient, diffuse, specular, shininess>
	vec3 lightdir = normalize(lightpos - fPosition);
	vec3 eyedir = normalize(campos - fPosition);

	vec3 normal = normalize(cross(fVelocity, eyedir));
	normal = normalize(cross(normal, fVelocity));

	//vec3 normal = normalize(-fPosition);

	//float d = max(0.0,dot(lightdir, normal));
	float d = abs(dot(lightdir, normal));
	float s = ceil(d)*pow(max(0.0,dot(reflect(-lightdir,normal), eyedir)),lightparams.w);

	vec3 cmag;
	if(mode == 0) { //ion
		cmag = vec3(0.0, 0.0, 1.0);
	} else if(mode == 1) { //electron
		cmag = vec3(1.0, 1.0, 0.0);
	} else {
		cmag = vec3(1.0,1.0,0.0);
	}

	color = vec4(min(vec3(1.0),(lightparams.r + lightparams.g*d)*cmag + s*lightparams.b), transparency);
	*/


	// ['#004499','#385a99','#536e97','#687f92','#7a8d8d','#8a9885','#99a07d','#a6a473','#b1a669','#baa55e',
	//  '#c1a152','#c59a46','#c7913a','#c7852f','#c47723','#be6718','#b6550f','#aa4007','#9c2803','#8b0000'] //blue->red(orange)
	
	vec3 COLOR_MASK[10];
	COLOR_MASK[0] = vec3(0.0, 0.267, 0.60);
	COLOR_MASK[1] = vec3(0.325, 0.431, 0.592);
	COLOR_MASK[2] = vec3(0.478, 0.553, 0.553);
	COLOR_MASK[3] = vec3(0.60, 0.627, 0.49);
	COLOR_MASK[4] = vec3(0.694, 0.651, 0.412);
	COLOR_MASK[5] = vec3(0.757, 0.631, 0.322);
	COLOR_MASK[6] = vec3(0.78, 0.569, 0.227);
	COLOR_MASK[7] = vec3(0.769, 0.467, 0.137);
	COLOR_MASK[8] = vec3(0.714, 0.333, 0.059);
	COLOR_MASK[9] = vec3(0.612, 0.157, 0.12);
	
	//['#0000bb','#3036bb','#4055b7','#486faf','#4b85a4','#4a9695','#46a583','#3eb06c','#31b750','#1bbb2a',
	// '#37bb00','#61b900','#7db400','#93ab00','#a39f00','#b08f00','#b87c00','#bd6300','#be4400','#bb0000']); //rainbow
	/*
	vec3 COLOR_MASK[10];
	COLOR_MASK[0] = vec3(0.0, 0.0, 0.733);
	COLOR_MASK[1] = vec3(0.251, 0.333, 0.718);
	COLOR_MASK[2] = vec3(0.294, 0.522, 0.643);
	COLOR_MASK[3] = vec3(0.275, 0.647, 0.514);
	COLOR_MASK[4] = vec3(0.192, 0.718, 0.314);
	COLOR_MASK[5] = vec3(0.216, 0.733, 0.0);
	COLOR_MASK[6] = vec3(0.49, 0.706, 0.0);
	COLOR_MASK[7] = vec3(0.639, 0.624, 0.0);
	COLOR_MASK[8] = vec3(0.722, 0.486, 0.0);
	COLOR_MASK[9] = vec3(0.745, 0.267, 0.0);
	*/

	float value;
	if(fVertex.w > 1.0) {
		value = 1.0;
	} else if(fVertex.w < 0.0) {
		value = 0.0;
	} else {
		value = fVertex.w;
	}
	vec3 rgb;

    if(coloring == 0) { //use above un/commented colormaps (COLORMASK)
		
		int lower = int(floor(value*9.0));
		int upper = int(ceil(value*9.0));
		float weight = value*9.0 - floor(value*9.0);
		rgb = (1.0-weight)*COLOR_MASK[lower] + weight*COLOR_MASK[upper];
		
		/*
        //HARD CODED RAINBOW
		if (value < 0.0) {
			rgb = vec3(0.0, 0.0, 0.5);
		} else if (0.0 <= value && value <= 0.125) {
			rgb = vec3(0.0, 0.0, 4.0*value + 0.5);
		} else if (0.125 < value && value <= 0.375) {
			rgb = vec3(0.0, 4.0*value - 0.5, 1.0);
		} else if (0.375 < value && value <= 0.625) {
			rgb = vec3(4.0*value - 1.5, 1.0, -4.0*value + 2.5);
		} else if (0.625 < value && value <= 0.875) {
			rgb = vec3(1.0, -4.0*value + 3.5, 0.0);
		} else if (0.875 < value && value <= 1.0) {
			rgb = vec3(-4.0*value + 4.5, 0.0, 0.0);
		} else {
			rgb = vec3(0.5, 0.0, 0.0);
		}
		rgb /= 1.2;
		*/

	} else {
		//BLUE WHITE RED
		if(value >= 0.5) {
			rgb = vec3(1.0, 2.0-2.0*value, 2.0-2.0*value);
		} else if(value < 0.5) {
			rgb = vec3(2.0*value, 2.0*value, 1.0);
		} else {
			rgb = vec3(0.0);
			//rgb = vec3(1.0);
		}
	}

	if(mode == 1) {

		// calculate normal from texture coordinates
		vec3 N;
		N.xy = gl_PointCoord* 2.0 - vec2(1.0);
		float mag = length(N.xy);
        if (mag > 1.0) discard;   // remove pixels outside circle
        N.z = sqrt(1.0-mag);

		// calculate lighting
        vec3 lightDir = vec3(0.1,-0.1,1.0);
		float diffuse = max(0.0, dot(normalize(lightDir), normalize(N)));

//        color = vec4(gl_PointCoord, 0.0, 1.0);
        color = vec4(rgb*diffuse, 1.0); //no transparancy
		//color = vec4(rgb*diffuse, value*0.9); //transparancy

	} else {
        color = vec4(rgb, 1.0);
	}
}

#version 330 core

struct Material
{
    vec3 emission;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};


struct Light
{
    vec3 position;
    vec3 direction;
    vec3 color;
    float angle;
};

const int numBoatLights=4;

//given from vertex shader
in vec3 tNormal;
in vec3 tFragPos;


out vec4 fragColor;


uniform Material uMaterial;
uniform Light uSpotlights[numBoatLights];
uniform vec3 uCameraPos;
uniform vec3 uLightDirectionalDir;
uniform vec3 uLightColor;
uniform float uKEmission;




//calculations light decaying.
vec3 calculateLightFalloff(vec3 lightColor, vec3 lightPosition, vec3 fragPosition) {

    float k_c = 1.0;
    float k_l = 0.14;
    float k_q = 0.07;
    float distance = length(lightPosition - fragPosition);

    return (1 / (k_c + k_l * distance + k_q * pow(distance, 2))) * lightColor;
}


//computes the diffuse and specular components for a given light source.
vec3 sumDiffSpec(float kDiffuse, vec3 diffMaterial, vec3 diffLightColor, vec3 nVector,
vec3 lVector, float kSpecular, vec3 specularMaterial, vec3 specLightColor,
vec3 vVector, float shininess) {

    lVector = normalize(lVector);
    vVector = normalize(vVector);
    nVector = normalize(nVector);
    vec3 hVector = normalize(lVector + vVector);


    vec3 diffuse = kDiffuse * diffMaterial * diffLightColor *  clamp(dot(nVector, lVector), 0, 1);
    vec3 specular =
    kSpecular * specularMaterial * specLightColor * pow( clamp(dot(nVector, hVector), 0, 1), shininess);

    return diffuse + specular;
}


vec3 calcSpotlightEffect(vec3 sumDiffuseSpecularLeftUpLight, vec3 inverseLVector, vec3 cVector, float cutOffAngle)
{
    inverseLVector = normalize(inverseLVector);
    cVector = normalize(cVector);

    float cosTheta = dot(inverseLVector, cVector);
    float tetha = acos(cosTheta);

    if(tetha > cutOffAngle)
    {
        sumDiffuseSpecularLeftUpLight = vec3(0, 0, 0);
    }
    else
    {
        sumDiffuseSpecularLeftUpLight = sumDiffuseSpecularLeftUpLight * pow(cosTheta,0.8);
    }

    return sumDiffuseSpecularLeftUpLight;
}

void main(void)
{
    //coefficients
    float kDiffuse = 1.0;
    float kSpecular = 1.0;
    float kAmbient = 0.1;
    float kEmission = uKEmission;
    vec3 ambientLight = uLightColor;
    vec3 diffLight = uLightColor;
    vec3 specLight = uLightColor;

    // ambient
    vec3 ambient = kAmbient * (uMaterial.ambient * uMaterial.diffuse) * ambientLight;

//    //emission
    vec3 emissionComponent = kEmission * uMaterial.emission;	// The emission component is multiplied by a factor, which is a uniform set at the CPU to either 0.0 or 0.8.


    // Diffuse and specular directional light.
    vec3 sumDiffSpecDirLight =
    sumDiffSpec(kDiffuse, uMaterial.diffuse, diffLight, tNormal, uLightDirectionalDir,
    kSpecular, uMaterial.specular, specLight, uCameraPos, uMaterial.shininess);

    // Diffuse and specular spotlights.
    vec3 sumSpotlightDiffSpec = vec3(0, 0, 0);

    for(int i = 0; i < numBoatLights; i++) {
        vec3 lightColor = calculateLightFalloff(uSpotlights[i].color, uSpotlights[i].position, tFragPos);

        vec3 sumDiffSpec = sumDiffSpec(
        kDiffuse, uMaterial.diffuse, lightColor, tNormal, uSpotlights[i].position - tFragPos, kSpecular,
        uMaterial.specular, uLightColor, uCameraPos, uMaterial.shininess);

        sumDiffSpec = calcSpotlightEffect(sumDiffSpec, tFragPos - uSpotlights[i].position,
        uSpotlights[i].direction, uSpotlights[i].angle);

        sumSpotlightDiffSpec += sumDiffSpec;

    }

    //adding all together
    vec3 totalDiffSpec = sumDiffSpecDirLight + sumSpotlightDiffSpec;
    vec3 result = ambient + totalDiffSpec;
    vec3 test= sumDiffSpecDirLight;


    fragColor = vec4(result, 1.0);


}


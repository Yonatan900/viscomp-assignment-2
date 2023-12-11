#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;
uniform float uTime;

out vec3 tNormal;
out vec3 tFragPos;
out vec2 tUV;

// Definiere Wellenparameter
const vec2 Direction1 = normalize(vec2(sqrt(2.0), sqrt(2.0)));
const vec2 Direction2 = normalize(vec2(sqrt(2.0), -sqrt(2.0)));
const vec2 Direction3 = normalize(vec2(-1.0, 0.0));

float calculateWaveHeight(float amplitude, float omega, float phi, vec2 direction, vec2 position)
{
    return amplitude * sin(dot(direction, position) * omega + uTime * phi);
}


void main(void)
{
    // Berechne die Höhe für jede Welle
    float waveHeight1 = calculateWaveHeight(0.6, 0.25, 0.5, Direction1, aPosition.xz);
    float waveHeight2 = calculateWaveHeight(0.7, 0.1, 0.25, Direction2, aPosition.xz);
    float waveHeight3 = calculateWaveHeight(0.1, 0.9, 0.9, Direction3, aPosition.xz);

    // Summiere die Wellenhöhen auf, um die endgültige Verschiebung zu erhalten
    float totalDisplacement = waveHeight1 + waveHeight2 + waveHeight3;

    // Aktualisiere die Vertexposition mit der Verschiebung
    vec4 displacedPosition = vec4(aPosition.x, aPosition.y + totalDisplacement, aPosition.z, 1.0);
    gl_Position = uProj * uView * uModel * displacedPosition;

    // Normalenberechnung
    float epsilon = 1.0;

    float dxHeight = calculateWaveHeight(0.6, 0.25, 0.5, Direction1, aPosition.xz + vec2(epsilon, 0.0)) +
    calculateWaveHeight(0.7, 0.1, 0.25, Direction2, aPosition.xz + vec2(epsilon, 0.0)) +
    calculateWaveHeight(0.1, 0.9, 0.9, Direction3, aPosition.xz + vec2(epsilon, 0.0));

    float dyHeight = calculateWaveHeight(0.6, 0.25, 0.5, Direction1, aPosition.xz + vec2(0.0, epsilon)) +
    calculateWaveHeight(0.7, 0.1, 0.25, Direction2, aPosition.xz + vec2(0.0, epsilon)) +
    calculateWaveHeight(0.1, 0.9, 0.9, Direction3, aPosition.xz + vec2(0.0, epsilon));

    // Vektorprodukt für die Normale
    vec3 normalApprox = normalize(vec3(dyHeight, epsilon, -dxHeight));

    // Übergebe andere Variablen an den Fragment-Shader
    tFragPos = vec3(uModel * displacedPosition);
    tUV = aUV;
    tNormal = normalApprox;
}

//void main(void)
//{
//    gl_Position = uProj * uView * uModel * vec4(aPosition, 1.0);
//    tFragPos = vec3(uModel * vec4(aPosition, 1.0));
//    tNormal = mat3(transpose(inverse(uModel))) * aNormal;
//    tUV = aUV;
//
//}

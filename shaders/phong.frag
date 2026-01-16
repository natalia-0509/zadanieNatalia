#version 330 core
in vec2 vUV;
in vec3 vNrm;
in vec3 vWorldPos;

out vec4 FragColor;

struct Material {
    vec3 Kd;   // możesz zostawić, ale najczęściej = (1,1,1)
    vec3 Ks;   // kolor połysku
    float Ns;  // shininess
};

uniform Material uMat;
uniform sampler2D uDiffuse;

uniform vec3 uViewPos;

uniform vec3 uLightDir;    // kierunek światła (z którego świeci)
uniform vec3 uLightColor;  // zwykle (1,1,1)

void main() {
    vec3 N = normalize(vNrm);
    vec3 L = normalize(-uLightDir);                 // "do światła"
    vec3 V = normalize(uViewPos - vWorldPos);

    vec3 albedo = texture(uDiffuse, vUV).rgb;       // kolor z tekstury

    // Ambient (żeby nie było czarno w cieniu)
    vec3 ambient = 0.30 * albedo;

    // Diffuse (Lambert)
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * albedo * uLightColor;

    // Specular (Phong)
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), max(uMat.Ns, 1.0));
    vec3 specular = spec * uMat.Ks * uLightColor;

    vec3 color = ambient + diffuse + specular;
    FragColor = vec4(color, 1.0);
}

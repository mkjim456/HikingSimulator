#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

void main() {
    float height = TexCoord.y;
    
    // Define more natural terrain colors
    vec4 snowColor = vec4(1.0, 1.0, 1.0, 1.0);          // White
    vec4 rockColor = vec4(0.5, 0.45, 0.40, 1.0);        // Grey-brown
    vec4 forestColor = vec4(0.2, 0.35, 0.1, 1.0);       // Dark green
    vec4 grassColor = vec4(0.3, 0.5, 0.15, 1.0);        // Light green
    vec4 groundColor = vec4(0.45, 0.38, 0.28, 1.0);     // Brown
    
    vec4 finalColor;
    
    if(height > 0.85) {
        finalColor = mix(rockColor, snowColor, (height - 0.85) / 0.15);
    }
    else if(height > 0.6) {
        finalColor = mix(forestColor, rockColor, (height - 0.6) / 0.25);
    }
    else if(height > 0.3) {
        finalColor = mix(grassColor, forestColor, (height - 0.3) / 0.3);
    }
    else {
        finalColor = mix(groundColor, grassColor, height / 0.3);
    }
    
    FragColor = finalColor;
}
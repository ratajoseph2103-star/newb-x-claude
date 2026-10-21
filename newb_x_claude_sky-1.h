// Newb X Claude
// Cielo realista de día y de noche, con estrellas en movimiento.
// Para shaders RenderDragon de Bedrock (dialecto bgfx/GLSL, ej. Newb X).
//
// Pensado para gama baja: 1 hash por pixel, unos pocos sin, sin loops,
// sin pow/exp/atan, y todo se puede apagar con los #define de abajo.
//
// USO: en el fragment del cielo reemplaza el color final por:
//     color = nxcSky(viewDir, sunDir, time, isEnd);
// - viewDir: dirección de la vista en espacio mundo, NORMALIZADA.
// - sunDir:  dirección hacia el sol, normalizada (la luna es -sunDir).
// - time:    segundos.
// - isEnd:   1.0 en el End, 0.0 en el Overworld.

#ifndef NXC_SKY_H
#define NXC_SKY_H

// ---- Ajustes ----
#define NXC_STARS          1      // estrellas (0 = apagar)
#define NXC_TWINKLE        1      // parpadeo de estrellas
#define NXC_MILKYWAY       1      // banda de la Vía Láctea
#define NXC_STAR_DENSITY   110.0  // más alto = más estrellas pequeñas
#define NXC_STAR_SPEED     0.01   // giro de las estrellas (rad/seg)
#define NXC_MOON           1      // luna con brillo

// Hash barato sin sin()
float nxc_hash(vec3 p) {
    vec3 q = fract(p * 0.1031);
    q += dot(q, q.yzx + 33.33);
    return fract((q.x + q.y) * q.z);
}

// Gira el cielo entero alrededor del eje X: las estrellas se mueven
vec3 nxc_rot(vec3 d, float time) {
    float a = time * NXC_STAR_SPEED;
    float c = cos(a);
    float s = sin(a);
    return vec3(d.x, d.y * c - d.z * s, d.y * s + d.z * c);
}

// Estrellas redondas con color y brillo variables
vec3 nxc_stars(vec3 p, float time) {
    vec3 g = p * NXC_STAR_DENSITY;
    vec3 f = fract(g) - 0.5;
    float h = nxc_hash(floor(g));
    float on = step(0.985, h);
    float shape = clamp(1.0 - dot(f, f) * 18.0, 0.0, 1.0);
    vec3 tint = mix(vec3(0.7, 0.8, 1.0), vec3(1.0, 0.82, 0.65), fract(h * 91.0));
    float bright = 0.5 + fract(h * 57.0) * 1.5;
#if NXC_TWINKLE
    bright *= 0.8 + 0.2 * sin(time * 2.5 + h * 80.0);
#endif
    return tint * shape * on * bright;
}

// ---------------- Overworld ----------------
vec3 nxcOverworldSky(vec3 d, vec3 sunDir, float time) {
    float h  = clamp(d.y, 0.0, 1.0);
    float t  = 1.0 - h;
    float t2 = t * t;
    float t4 = t2 * t2;

    // Día/noche suave según la altura del sol
    float dayf = clamp(sunDir.y * 4.0 + 0.5, 0.0, 1.0);
    dayf = dayf * dayf * (3.0 - 2.0 * dayf);

    // Día: degradado Rayleigh falso + brillo del sol
    float sd = dot(d, sunDir);
    vec3 dayCol = mix(vec3(0.20, 0.42, 0.85), vec3(0.72, 0.84, 0.95), t4);
    dayCol += vec3(1.0, 0.85, 0.6) * (0.25 / (1.0 + 30.0 * (1.0 - sd)));
    dayCol += vec3(1.0, 0.95, 0.85) * clamp((sd - 0.9990) * 2000.0, 0.0, 1.0);

    // Amanecer/atardecer: tinte cálido hacia el sol
    float sunLow = 1.0 - clamp(abs(sunDir.y) * 3.0, 0.0, 1.0);
    vec3 warm = vec3(1.0, 0.45, 0.2) * sunLow * t2 * (0.4 + 0.6 * clamp(sd, 0.0, 1.0));

    // Noche: azul muy oscuro con un poco de neblina en el horizonte
    vec3 nightCol = mix(vec3(0.004, 0.008, 0.025), vec3(0.02, 0.03, 0.06), t4);
    float nightf = 1.0 - dayf;

    vec3 col = mix(nightCol, dayCol, dayf) + warm;

    vec3 p = nxc_rot(d, time);
    float horizonFade = clamp(d.y * 3.0, 0.0, 1.0);

#if NXC_MILKYWAY
    // Vía Láctea: banda inclinada con manchas
    float band = dot(p, vec3(0.30, 0.80, 0.50));
    float mw = 1.0 / (1.0 + band * band * 30.0);
    float n = 0.6 + 0.4 * sin(p.x * 9.0 + sin(p.z * 7.0) * 2.0) * sin(p.y * 11.0);
    col += vec3(0.50, 0.55, 0.80) * mw * n * 0.10 * nightf * horizonFade;
#endif

#if NXC_STARS
    col += nxc_stars(p, time) * nightf * nightf * horizonFade;
#endif

#if NXC_MOON
    // Luna opuesta al sol
    float md = -sd;
    float moonUp = clamp(-sunDir.y * 4.0 + 0.5, 0.0, 1.0);
    col += vec3(0.75, 0.82, 1.0) * clamp((md - 0.9993) * 4000.0, 0.0, 1.0) * moonUp;
    col += vec3(0.30, 0.38, 0.60) * (0.10 / (1.0 + 60.0 * (1.0 - md))) * moonUp;
#endif

    return col;
}

// ---------------- End: agujero negro ----------------
#define NXC_BH_R2    0.09   // radio del agujero al cuadrado (0.3 * 0.3)
#define NXC_BH_W     0.85   // ancho del disco
#define NXC_BH_H     0.12   // grosor del disco
#define NXC_BH_SPIN  0.8    // movimiento del disco
#define NXC_BH_DRIFT 0.02   // deriva lenta por el cielo

vec3 nxcEndSky(vec3 d, float time) {
    vec3 col = mix(vec3(0.01, 0.0, 0.02), vec3(0.05, 0.02, 0.10), d.y * 0.5 + 0.5);

#if NXC_STARS
    col += nxc_stars(nxc_rot(d, time), time) * 0.9;
#endif

    // Coordenadas planas hacia -Z (sin cross/normalize)
    float fac = -d.z;
    float on  = step(0.05, fac);
    float inv = 1.0 / max(fac, 0.05);
    float tx = d.x * inv - sin(time * NXC_BH_DRIFT) * 0.08;
    float ty = d.y * inv - 0.18;
    float r2 = tx * tx + ty * ty;

    // Halo naranja
    float halo = clamp(1.0 - abs(r2 - NXC_BH_R2 * 1.25) * 14.0, 0.0, 1.0);
    col = mix(col, vec3(1.0, 0.7, 0.3), halo * 0.8 * on);

    // Agujero negro
    col = mix(col, vec3(0.0), step(r2, NXC_BH_R2) * on);

    // Disco de acreción (la mitad de atrás queda oculta por el agujero)
    float ex = tx * (1.0 / NXC_BH_W);
    float ey = ty * (1.0 / NXC_BH_H);
    float e2 = ex * ex + ey * ey;
    float vis = max(step(NXC_BH_R2, r2), step(0.0, ty));
    float a = clamp((1.0 - e2) * 3.0, 0.0, 1.0) * vis * on;
    float shimmer = 0.75 + 0.25 * sin(tx * 18.0 - time * NXC_BH_SPIN * 3.0);
    vec3 diskCol = mix(vec3(1.0, 0.45, 0.12), vec3(1.0, 0.95, 0.7), clamp(1.0 - e2, 0.0, 1.0)) * shimmer;
    col = mix(col, diskCol, a);

    return col;
}

// ---------------- Selector ----------------
vec3 nxcSky(vec3 viewDir, vec3 sunDir, float time, float isEnd) {
    return isEnd > 0.5 ? nxcEndSky(viewDir, time) : nxcOverworldSky(viewDir, sunDir, time);
}

#endif

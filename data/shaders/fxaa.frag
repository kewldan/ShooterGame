#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

// FXAA 3.11 (Timothy Lottes), the PC "quality" path with preset 12: an early exit on low local contrast,
// an edge direction estimate from the 3x3 neighbourhood, an end-of-edge search in up to 5 steps of
// growing length in both directions and a blend towards the neighbour across the edge; plus the sub-pixel
// aliasing filter for isolated bright pixels. Reads the luma from the alpha channel (written by
// tonemap.frag) so no extra conversion is needed per tap.
uniform sampler2D source;
uniform vec2 invSize; // 1 / size of `source`

const float EDGE_THRESHOLD = 0.125;     // minimum local contrast to filter (lower = more edges)
const float EDGE_THRESHOLD_MIN = 0.0625; // trims dark areas
const float SUBPIX = 0.75;              // sub-pixel aliasing removal, 0 = off, 1 = softest
const int STEPS = 5;
const float STEP_SIZES[STEPS] = float[](1.0, 1.5, 2.0, 4.0, 12.0);

float luma(vec2 uv)
{
    return texture(source, uv).a;
}

void main()
{
    vec2 posM = TexCoords;
    vec4 rgbyM = texture(source, posM);
    float lumaM = rgbyM.a;
    float lumaS = luma(posM + vec2( 0.0,  1.0) * invSize);
    float lumaE = luma(posM + vec2( 1.0,  0.0) * invSize);
    float lumaN = luma(posM + vec2( 0.0, -1.0) * invSize);
    float lumaW = luma(posM + vec2(-1.0,  0.0) * invSize);

    float maxSM = max(lumaS, lumaM), minSM = min(lumaS, lumaM);
    float maxESM = max(lumaE, maxSM), minESM = min(lumaE, minSM);
    float maxWN = max(lumaN, lumaW), minWN = min(lumaN, lumaW);
    float rangeMax = max(maxWN, maxESM), rangeMin = min(minWN, minESM);
    float range = rangeMax - rangeMin;
    if (range < max(EDGE_THRESHOLD_MIN, rangeMax * EDGE_THRESHOLD)) {
        FragColor = rgbyM;
        return;
    }

    float lumaNW = luma(posM + vec2(-1.0, -1.0) * invSize);
    float lumaSE = luma(posM + vec2( 1.0,  1.0) * invSize);
    float lumaNE = luma(posM + vec2( 1.0, -1.0) * invSize);
    float lumaSW = luma(posM + vec2(-1.0,  1.0) * invSize);

    float lumaNS = lumaN + lumaS, lumaWE = lumaW + lumaE;
    float subpixRcpRange = 1.0 / range;
    float subpixNSWE = lumaNS + lumaWE;
    float edgeHorz1 = -2.0 * lumaM + lumaNS;
    float edgeVert1 = -2.0 * lumaM + lumaWE;

    float lumaNESE = lumaNE + lumaSE, lumaNWNE = lumaNW + lumaNE;
    float edgeHorz2 = -2.0 * lumaE + lumaNESE;
    float edgeVert2 = -2.0 * lumaN + lumaNWNE;

    float lumaNWSW = lumaNW + lumaSW, lumaSWSE = lumaSW + lumaSE;
    float edgeHorz4 = abs(edgeHorz1) * 2.0 + abs(edgeHorz2);
    float edgeVert4 = abs(edgeVert1) * 2.0 + abs(edgeVert2);
    float edgeHorz3 = -2.0 * lumaW + lumaNWSW;
    float edgeVert3 = -2.0 * lumaS + lumaSWSE;
    float edgeHorz = abs(edgeHorz3) + edgeHorz4;
    float edgeVert = abs(edgeVert3) + edgeVert4;

    float subpixNWSWNESE = lumaNWSW + lumaNESE;
    float lengthSign = invSize.x;
    bool horzSpan = edgeHorz >= edgeVert;
    float subpixA = subpixNSWE * 2.0 + subpixNWSWNESE;

    if (!horzSpan) lumaN = lumaW;
    if (!horzSpan) lumaS = lumaE;
    if (horzSpan) lengthSign = invSize.y;
    float subpixB = subpixA * (1.0 / 12.0) - lumaM;

    float gradientN = lumaN - lumaM, gradientS = lumaS - lumaM;
    float lumaNN = lumaN + lumaM, lumaSS = lumaS + lumaM;
    bool pairN = abs(gradientN) >= abs(gradientS);
    float gradient = max(abs(gradientN), abs(gradientS));
    if (pairN) lengthSign = -lengthSign;
    float subpixC = clamp(abs(subpixB) * subpixRcpRange, 0.0, 1.0);

    vec2 posB = posM;
    vec2 offNP = vec2(horzSpan ? invSize.x : 0.0, horzSpan ? 0.0 : invSize.y);
    if (!horzSpan) posB.x += lengthSign * 0.5;
    if (horzSpan) posB.y += lengthSign * 0.5;

    vec2 posN = posB - offNP * STEP_SIZES[0];
    vec2 posP = posB + offNP * STEP_SIZES[0];
    float subpixD = -2.0 * subpixC + 3.0;
    float lumaEndN = luma(posN);
    float subpixE = subpixC * subpixC;
    float lumaEndP = luma(posP);

    if (!pairN) lumaNN = lumaSS;
    float gradientScaled = gradient * 0.25;
    float lumaMM = lumaM - lumaNN * 0.5;
    float subpixF = subpixD * subpixE;
    bool lumaMLTZero = lumaMM < 0.0;

    lumaEndN -= lumaNN * 0.5;
    lumaEndP -= lumaNN * 0.5;
    bool doneN = abs(lumaEndN) >= gradientScaled;
    bool doneP = abs(lumaEndP) >= gradientScaled;
    if (!doneN) posN -= offNP * STEP_SIZES[1];
    if (!doneP) posP += offNP * STEP_SIZES[1];

    // Walk along the edge until the luma drifts off the pair's average in both directions.
    for (int i = 2; i < STEPS && (!doneN || !doneP); i++) {
        if (!doneN) lumaEndN = luma(posN) - lumaNN * 0.5;
        if (!doneP) lumaEndP = luma(posP) - lumaNN * 0.5;
        doneN = abs(lumaEndN) >= gradientScaled;
        doneP = abs(lumaEndP) >= gradientScaled;
        if (!doneN) posN -= offNP * STEP_SIZES[i];
        if (!doneP) posP += offNP * STEP_SIZES[i];
    }

    float dstN = horzSpan ? posM.x - posN.x : posM.y - posN.y;
    float dstP = horzSpan ? posP.x - posM.x : posP.y - posM.y;

    bool goodSpanN = (lumaEndN < 0.0) != lumaMLTZero;
    bool goodSpanP = (lumaEndP < 0.0) != lumaMLTZero;
    float spanLength = dstP + dstN;
    float spanLengthRcp = 1.0 / spanLength;

    bool directionN = dstN < dstP;
    float dst = min(dstN, dstP);
    bool goodSpan = directionN ? goodSpanN : goodSpanP;
    float subpixG = subpixF * subpixF;
    float pixelOffset = dst * -spanLengthRcp + 0.5;
    float subpixH = subpixG * SUBPIX;

    float pixelOffsetGood = goodSpan ? pixelOffset : 0.0;
    float pixelOffsetSubpix = max(pixelOffsetGood, subpixH);
    if (!horzSpan) posM.x += pixelOffsetSubpix * lengthSign;
    if (horzSpan) posM.y += pixelOffsetSubpix * lengthSign;

    FragColor = vec4(texture(source, posM).rgb, lumaM);
}

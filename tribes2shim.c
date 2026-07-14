#define _GNU_SOURCE

int SDL_GetGamma(float *red, float *green, float *blue)
{
    if (red)
        *red = 1.0f;

    if (green)
        *green = 1.0f;

    if (blue)
        *blue = 1.0f;

    return 0;
}

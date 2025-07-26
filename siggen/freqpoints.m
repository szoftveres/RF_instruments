
ranges = 144;  % Multiples of 9 gives good boundaries

flow = 23500; % 23.5 MHz inclusive
fhigh = 6000000.001; % 6GHz inclusive


fstart = fhigh / 512
fstop = fhigh;

boundaries = logspace(log10(fstart), log10(fstop), ranges + 1);


printf("range start {\n");
for p = 1:ranges
    if (boundaries(p) > flow)
        printf("   %i,\n", floor(boundaries(p)));
    endif
end
printf("}\n");


printf("calibration points {\n");
for p = 1:ranges
    midfreq = sqrt(boundaries(p) * boundaries(p+1));
    printf("   %i,\n", floor(midfreq));
end
printf("}\n");


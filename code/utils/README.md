..:: DistCalc Bench ::..

Command-line utility for benchmarking distance calculation methods to be used with LibLRS.

How to build: run 'build.bat' from a terminal. This utility requires GDAL, so you will need to edit the paths for the GDAL include files and lib in the build.

Arguments: -h (--help) prints out the instructions; -s (--shapefile) specifies the path to the LinestringZ shapefile to be used for distance calculations (must be in latlon, mandatory); -e (--epsg) specifies the EPSG code for the transformation to be used for computer projected distances (mandatory); -l (--loop) tells the utility how many times to loop over each distance method, to acquire more faithful timing (optional, default 100).

Output: written to stdout, a table with columns [calc method] [distance] {preparation} {processing} {total}. Each of the last three columns is a tuple with three values, being the lowest, average, and highest times for the loops, in microseconds. So, a tuple of { 10, 20, 45} means that, withing the N loops of that run, the lowest time was 10 microseconds and the highest was 45, with an average of 20. The {preparation} column measures the time it takes to prepare the geometry for processing (including reprojecting), and {processing} is the time needed to calculate the total distance. The column {total} is a sum of the two.

Example output:
```
         Projected 3D: 106561.9821     {  539.4,   687.9,  3434.7}     {    8.8,     9.4,    30.0}     {  548.2,   697.3,  3464.7}
         Projected 2D: 106526.7568     {  538.2,   642.1,  2351.2}     {    6.6,     6.9,    14.4}     {  544.8,   649.0,  2365.6}
  Ellipsoidal Cart 3D: 106570.8172     {   55.8,    69.0,   327.8}     {    8.8,    10.9,    40.9}     {   64.6,    79.9,   368.7}
  Ellipsoidal Cart 2D: 106532.0066     {   54.3,    55.6,   104.0}     {    8.8,     9.2,    25.0}     {   63.1,    64.8,   129.0}
      Ellipsoidal Arc: 106424.7740     {   55.7,    69.8,   341.3}     {   63.5,    72.0,   200.3}     {  119.2,   141.8,   541.6}
        Spherical Arc: 106757.1516     {   67.2,    82.1,   189.1}     {   37.5,    43.9,   120.7}     {  104.7,   126.0,   309.8}
            Haversine: 106757.1452     {    2.8,     3.7,    24.9}     {  273.0,   305.1,   552.9}     {  275.8,   308.8,   577.8}
```
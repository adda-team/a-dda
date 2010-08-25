/* File: vars.c
 * $Author$
 * $Date::                            $
 * Descr: all the global variables are declared here
 *
 *        'Global' means used in three or more source files. Variables that are used in only two
 *        source files are called 'semi-global' and not listed here. They are defined in one file
 *        and referenced with 'extern' in another one.
 *
 * Copyright (C) 2006-2008 University of Amsterdam
 * Copyright (C) 2009,2010 Institute of Chemical Kinetics and Combustion & University of Amsterdam
 * This file is part of ADDA.
 *
 * ADDA is free software: you can redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * ADDA is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
 * the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with ADDA. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>  // for FILE and size_t
#include <time.h>   // for time_t
#include "const.h"  // for MAX_NMAT, MAX_DIRNAME
#include "types.h"  // for doublecomplex, angle_set, scat_grid_angles
#include "timing.h" // for TIME_TYPE

// basic variables
int boxX,boxY,boxZ;       // sizes of box enclosing the particle
double dipvol;            // dipole volume
double kd;                // k*d=2*PI/dpl
double ka_eq;             // volume-equivalent size parameter
double inv_G;             // inverse of equivalent cross section
double WaveNum;           // wavenumber of incident light
double *DipoleCoord;      // vector to hold the coordinates of the dipoles
unsigned short *position; /* position of the dipoles; in the very end of make_particle()
                           * z-components are adjusted to be relative to the local_z0
                           */
double memory;            // total memory usage in bytes
enum inter IntRelation;   // type of formula for interaction term
enum pol PolRelation;     // type of formula for self-term (polarization relation)
enum beam  beamtype;      // type of incident beam

// symmetries
bool symX,symY,symZ; /* symmetries of reflection relative to the planes perpendicular to x, y, and
                     * z axes. Only Y is actually used
                     */
bool symR;           // symmetry of 90-degrees rotation about z axes

// flags (true or false)
bool prognosis;     // make a prognosis about needed ram
bool yzplane;      // Calculate the field in the yz-plane
bool all_dir;      /* Calculate the field for all directions on a theta-phi grid (internal
                   * parameter - initialized by other options: calculation of Csca and asym)
                   */
bool scat_grid;    // calculate field on a grid of scattering angles
bool phi_integr;   // integrate over the phi angle
bool reduced_FFT;  // reduced number of storage for FFT, when matrix is symmetric
bool orient_avg;   // whether to use orientation averaging
bool load_chpoint; // whether to load checkpoint
bool beam_asym;    // whether the beam center is shifted relative to the origin
bool sh_granul;    // whether to fill one domain with granules
bool anisotropy;   // whether the scattering medium is anisotropic
bool save_memory;  // whether to sacrifice some speed for memory

// 3D vectors (in particle reference frame)
double prop[3];               // incident direction (in particle reference frame)
double incPolX[3],incPolY[3]; // incident polarizations (in particle RF)
double beam_center[3];        // coordinates of the beam center
double box_origin_unif[3];    /* coordinates of the center of the first dipole in the local
                               * computational box (after uniform distribution of non-void dipoles
                               * among all processors)
                               */

// file info
char directory[MAX_DIRNAME]; // directory to save data in
FILE *logfile;               // file where all the information about the run is saved
int term_width;              // width of the terminal to which ADDA produces output

// refractive index
int Nmat;                           /* number of different domains (for each either scalar or
                                     * tensor refractive index is specified
                                     */
int Ncomp;                          // number of components of each refractive index (1 or 3)
doublecomplex ref_index[MAX_NMAT];  // a set of refractive indexes
doublecomplex cc_sqrt[MAX_NMAT][3]; // sqrt of couple constants
doublecomplex chi_inv[MAX_NMAT][3]; // normalized inverse susceptibility: = 1/(V*chi)
unsigned char *material;            // material: index for cc

// iterative solver
enum iter IterMethod; // iterative method to use
int maxiter;          // maximum number of iterations
doublecomplex *xvec;  // total electric field on the dipoles
doublecomplex *pvec;  // polarization of dipoles, also an auxiliary vector in iterative solvers
doublecomplex *Einc;  // incident field on dipoles

// scattering at different angles
int nTheta;                        // number of angles in scattering profile
double alph_deg, bet_deg, gam_deg; // Euler angles of particle orientation in degrees
angle_set alpha_int;               // sets of angles
scat_grid_angles angles;           // angle sets for scat_grid
doublecomplex *EgridX,*EgridY;     /* E calculated on a grid for many different directions (holds
                                    * Eper and Epar) for two incident polarizations
                                    */
double *Egrid_buffer;              // buffer to accumulate Egrid

// checkpoint
enum chpoint chp_type;              // type of checkpoint (to save)
time_t chp_time;           // time of checkpoint (in sec)
char chp_dir[MAX_DIRNAME]; // directory name to save/load checkpoint

// auxiliary grids and their partition over processors
size_t gridX,gridY,gridZ;          /* sizes of the 'matrix' X, size_t - to remove type conversions
                                    * we assume that 'int' is enough for it, but this declaration is
                                    * to avoid type casting in calculations
                                    */
size_t gridYZ;                     // gridY*gridZ
size_t smallY,smallZ;              // the size of the reduced matrix X
size_t local_Nsmall;               // number of  points of expanded grid per one processor
int nprocs;                        // total number of processes
int ringid;                        // ID of current process
int local_z0,local_z1;             // starting and ending z for current processor
size_t local_Nz;                   // number of z layers (based on the division of smallZ)
int local_Nz_unif;                 /* number of z layers (distance between max and min values),
                                    * belonging to this processor, after all non_void dipoles are
                                    * uniformly distributed between all processors
                                    */
int local_z1_coer;                 // ending z, coerced to be not greater than boxZ
size_t local_x0,local_x1,local_Nx; /* starting, ending x for current processor and number of x
                                    * layers (based on the division of smallX)
                                    */
size_t local_Ndip;                 // number of local total dipoles
size_t local_nvoid_Ndip;           // number of local and ...
double nvoid_Ndip;                 // ... total non-void dipoles
size_t nlocalRows;                 // number of local rows of decomposition (only real dipoles)

// timing
time_t wt_start,              // starting wall time
       last_chp_wt;           // wall time of the last checkpoint
TIME_TYPE Timing_EField,      // time for calculating scattered fields
          Timing_FileIO,      // time for input and output
          Timing_Integration, // time for all integrations (with precomputed values)
          tstart_main;        // starting time of the program (after MPI_Init in parallel)

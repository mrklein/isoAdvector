/*---------------------------------------------------------------------------*\

License
    This file is part of IsoAdvector, which is an unofficial extension to
    OpenFOAM.

    IsoAdvector is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    IsoAdvector is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with IsoAdvector.  If not, see <http://www.gnu.org/licenses/>.

Application
    isoAdvector

Description
    Advects a volume of fluid across an FVM mesh by fluxing fluid through its
    faces. Fluid transport across faces during a time step is estimated from
    the cell cutting of isosurfaces of the VOF field.

Author
    Johan Roenby, DHI, all rights reserved.


\*---------------------------------------------------------------------------*/

#include "fvCFD.H"
/*
#include "CMULES.H"
#include "EulerDdtScheme.H"
#include "localEulerDdtScheme.H"
#include "CrankNicolsonDdtScheme.H"
#include "subCycle.H"
#include "immiscibleIncompressibleTwoPhaseMixture.H"
#include "turbulentTransportModel.H"
#include "pimpleControl.H"
#include "CorrectPhi.H"
#include "localEulerDdtScheme.H"
#include "fvcSmooth.H"
*/
#include "fvOptions.H"
#include "isoAdvection.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    #include "postProcess.H"

    #include "setRootCase.H"
    #include "createTime.H"
    #include "createMesh.H"
    #include "createControl.H"
    #include "createTimeControls.H"
//    #include "createRDeltaT.H"
//    #include "initContinuityErrs.H"
    #include "createFields.H"
    #include "createFvOptions.H"
//    #include "correctPhi.H"

    #include "readTimeControls.H"
    #include "CourantNo.H"
    #include "setInitialDeltaT.H"

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;
    scalar executionTime = runTime.elapsedCpuTime();

    while (runTime.run())
    {
        #include "readTimeControls.H"

        #include "CourantNo.H"
        #include "alphaCourantNo.H"
        #include "setDeltaT.H"

        //Setting velocity field and face fluxes for next time step
        scalar t = runTime.time().value();
        scalar dt = runTime.deltaT().value();
        if ( reverseTime > 0.0 && t >= reverseTime )
        {
            Info<< "Reversing flow" << endl;
            phi = -phi;
            phi0 = -phi0;
            U = -U;
            U0 = -U0;
            reverseTime = -1.0;
        }
        if ( period > 0.0 )
        {
            phi = phi0*Foam::cos(2.0*M_PI*(t + 0.5*dt)/period);
            U = U0*Foam::cos(2.0*M_PI*(t + 0.5*dt)/period);
        }

        runTime++;

        Info<< "Time = " << runTime.timeName() << nl << endl;

        //Advance alpha1 from time t to t+dt
        advector.advect();
//            #include "alphaControls.H"
//            #include "alphaEqnSubCycle.H"

        //Clip and snap alpha1 to ensure strict boundedness to machine precision
        if ( clipAlphaTol > 0.0 )
        {
            alpha1 = alpha1*
                pos(alpha1-clipAlphaTol)*neg(alpha1-(1.0-clipAlphaTol))
                + pos(alpha1-(1.0-clipAlphaTol));
        }
        if ( snapAlpha )
        {
            alpha1 = min(1.0,max(0.0,alpha1));
        }

        const scalar V = gSum(mesh.V().field()*alpha1.internalField());
        Info << "t = " << runTime.time().value() << ",\t sum(alpha*V) = " << V
             << ",\t dev = " << 100*(1.0 - V/V0) << "%"
             << ",\t 1-max(alpha1) = " << 1 - gMax(alpha1.internalField())
             << ",\t min(alpha1) = " << gMin(alpha1.internalField())
             << endl;

        if (printSurfCells)
        {
            advector.getSurfaceCells(surfCells);
        }

       if (printBoundCells)
        {
            advector.getBoundedCells(boundCells);
        }

        runTime.write();

        scalar newExecutionTime = runTime.elapsedCpuTime();
        Info<< "ExecutionTime = " << newExecutionTime << " s,"
            << " ClockTime = " << runTime.elapsedClockTime() << " s,"
            << " timeStepTime = " << newExecutionTime - executionTime << " s,"
            << nl << endl;
        executionTime = runTime.elapsedCpuTime();
    }

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //

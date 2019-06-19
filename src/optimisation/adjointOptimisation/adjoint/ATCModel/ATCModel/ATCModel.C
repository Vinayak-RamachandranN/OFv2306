/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2019 OpenCFD Ltd.
     \\/     M anipulation  |
-------------------------------------------------------------------------------
                            | Copyright (C) 2007-2019 PCOpt/NTUA
                            | Copyright (C) 2013-2019 FOSS GP
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "ATCModel.H"
#include "localMin.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(ATCModel, 0);
defineRunTimeSelectionTable(ATCModel, dictionary);

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void ATCModel::computeLimiter()
{
    computeLimiter(ATClimiter_, zeroATCcells_->getZeroATCcells(), nSmooth_);
}


void ATCModel::smoothATC()
{
    ATC_ *= ATClimiter_;
    DebugInfo<<
        "max ATC mag " << gMax(ATC_) << endl;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

ATCModel::ATCModel
(
    const fvMesh& mesh,
    const incompressibleVars& primalVars,
    const incompressibleAdjointVars& adjointVars,
    const dictionary& dict
)
:
    regIOobject
    (
        IOobject
        (
            "ATCModel" + adjointVars.solverName(),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        )
    ),
    mesh_(mesh),
    primalVars_(primalVars),
    adjointVars_(adjointVars),
    dict_(dict),
    extraConvection_(dict_.lookupOrDefault<scalar>("extraConvection", Zero)),
    extraDiffusion_(dict_.lookupOrDefault<scalar>("extraDiffusion", Zero)),
    nSmooth_(dict_.lookupOrDefault<label>("nSmooth", 0)),
    reconstructGradients_
    (
        dict_.lookupOrDefault<bool>("reconstructGradients", false)
    ),
    adjointSolverName_(adjointVars.solverName()),
    zeroATCcells_(zeroATCcells::New(mesh, dict_)),
    ATClimiter_
    (
        IOobject
        (
            "ATClimiter" + adjointSolverName_,
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedScalar("limiter", dimless, 1.0),
        zeroGradientFvPatchField<scalar>::typeName
    ),
    ATC_
    (
        IOobject
        (
            "ATCField" + adjointSolverName_,
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedVector(dimensionSet(0, 1, -2, 0, 0), Zero)
    )
{
    // Compute ATC limiter
    computeLimiter();
}


// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

autoPtr<ATCModel> ATCModel::New
(
    const fvMesh& mesh,
    const incompressibleVars& primalVars,
    const incompressibleAdjointVars& adjointVars,
    const dictionary& dict
)
{
    const word modelType(dict.get<word>("ATCModel"));

    auto cstrIter = dictionaryConstructorTablePtr_->cfind(modelType);

    Info<< "ATCModel type " << modelType << endl;

    if (!cstrIter.found())
    {
        FatalIOErrorInFunction(dict)
            << "Unknown ATCModel type " << modelType << nl << nl
            << "Valid ATCModel types are :" << nl
            << dictionaryConstructorTablePtr_->sortedToc()
            << exit(FatalIOError);
    }

    return autoPtr<ATCModel>
    (
        cstrIter()(mesh, primalVars, adjointVars, dict)
    );
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const labelList& ATCModel::getZeroATCcells()
{
    return zeroATCcells_->getZeroATCcells();
}


scalar ATCModel::getExtraConvectionMultiplier()
{
    return extraConvection_;
}


scalar ATCModel::getExtraDiffusionMultiplier()
{
    return extraDiffusion_;
}


const volScalarField& ATCModel::getLimiter() const
{
    return ATClimiter_;
}


void ATCModel::computeLimiter
(
    volScalarField& limiter,
    const labelList& cells,
    const label nSmooth
)
{
    // Restore values
    limiter.primitiveFieldRef() = 1;
    limiter.correctBoundaryConditions();

    // Set to zero in predefined cells
    for (const label celli : cells)
    {
        limiter[celli] = Zero;
    }

    // Correct bcs to get the correct value for boundary faces
    limiter.correctBoundaryConditions();

    // Apply "laplacian" smoother
    const fvMesh& mesh = limiter.mesh();
    const localMin<scalar> scheme(mesh);
    for (label iLimit = 0; iLimit < nSmooth; ++iLimit)
    {
        surfaceScalarField faceLimiter
        (
            scheme.interpolate(limiter)
        );
        limiter = fvc::average(faceLimiter);
    }
}


tmp<volScalarField> ATCModel::createLimiter
(
    const fvMesh& mesh,
    const dictionary& dict
)
{
    autoPtr<zeroATCcells> zeroType(zeroATCcells::New(mesh, dict));
    const labelList& zeroCells = zeroType->getZeroATCcells();
    const label nSmooth = dict.lookupOrDefault<label>("nSmooth", 0);

    tmp<volScalarField> tlimiter
    (
        new volScalarField
        (
           IOobject
           (
               "limiter",
               mesh.time().timeName(),
               mesh,
               IOobject::NO_READ,
               IOobject::NO_WRITE
           ),
           mesh,
           dimensionedScalar("limiter", dimless, 1.0),
           zeroGradientFvPatchField<scalar>::typeName
        )
    );
    volScalarField& limiter = tlimiter.ref();

    computeLimiter(limiter, zeroCells, nSmooth);

    return tlimiter;
}


bool ATCModel::writeData(Ostream&) const
{
    // Does nothing
    return true;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
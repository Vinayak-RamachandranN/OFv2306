/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2013 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

Class
    Foam::cyclicGAMGInterfaceField

Description
    GAMG agglomerated cyclic interface field.

SourceFiles
    cyclicGAMGInterfaceField.C

\*---------------------------------------------------------------------------*/

#ifndef cyclicGAMGInterfaceField_H
#define cyclicGAMGInterfaceField_H

#include "GAMGInterfaceField.H"
#include "cyclicGAMGInterface.H"
#include "cyclicLduInterfaceField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                  Class cyclicGAMGInterfaceField Declaration
\*---------------------------------------------------------------------------*/

class cyclicGAMGInterfaceField
:
    public GAMGInterfaceField,
    virtual public cyclicLduInterfaceField
{
    // Private data

        //- Local reference cast into the cyclic interface
        const cyclicGAMGInterface& cyclicInterface_;

        //- Is the transform required
        bool doTransform_;

        //- Rank of component for transformation
        int rank_;


    // Private Member Functions

        //- No copy construct
        cyclicGAMGInterfaceField(const cyclicGAMGInterfaceField&) = delete;

        //- No copy assignment
        void operator=(const cyclicGAMGInterfaceField&) = delete;


public:

    //- Runtime type information
    TypeName("cyclic");


    // Constructors

        //- Construct from GAMG interface and fine level interface field
        cyclicGAMGInterfaceField
        (
            const GAMGInterface& GAMGCp,
            const lduInterfaceField& fineInterfaceField
        );

        //- Construct from GAMG interface and fine level interface field
        cyclicGAMGInterfaceField
        (
            const GAMGInterface& GAMGCp,
            const bool doTransform,
            const int rank
        );

        //- Construct from GAMG interface and Istream
        cyclicGAMGInterfaceField
        (
            const GAMGInterface& GAMGCp,
            Istream& is
        );

        //- Construct from GAMG interface and local and remote fields
        cyclicGAMGInterfaceField
        (
            const GAMGInterface& GAMGCp,
            const lduInterfaceField& local, // local
            const UPtrList<lduInterfaceField>& other // other
        );

        //- Construct by assembling and return a clone.
        virtual autoPtr<GAMGInterfaceField> clone
        (
            const GAMGInterface& GAMGCp,
            const UPtrList<lduInterfaceField>& other // other
        ) const
        {
            return autoPtr<GAMGInterfaceField>
            (
                new cyclicGAMGInterfaceField
                (
                    GAMGCp,
                    *this,      // local field
                    other       // other fields
                )
            );
        }


    //- Destructor
    virtual ~cyclicGAMGInterfaceField() = default;


    // Member Functions

        // Access

            //- Return size
            label size() const
            {
                return cyclicInterface_.size();
            }


        //- Cyclic interface functions

            //- Does the interface field perform the transformation
            virtual bool doTransform() const
            {
                return doTransform_;
            }

            //- Return face transformation tensor
            virtual const tensorField& forwardT() const
            {
                return cyclicInterface_.forwardT();
            }

            //- Return neighbour-cell transformation tensor
            virtual const tensorField& reverseT() const
            {
                return cyclicInterface_.reverseT();
            }

            //- Return rank of component for transform
            virtual int rank() const
            {
                return rank_;
            }


        // Interface matrix update

            //- Update result field based on interface functionality
            virtual void updateInterfaceMatrix
            (
                solveScalarField& result,
                const bool add,
                const lduAddressing& lduAddr,
                const label patchId,
                const solveScalarField& psiInternal,
                const scalarField& coeffs,
                const direction cmpt,
                const Pstream::commsTypes commsType
            ) const;


        // I/O

            //- Write to stream
            virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

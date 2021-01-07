/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2019-2020 OpenCFD Ltd.
------------------------------------------------------------------------------
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
    Foam::fa::optionList

Description
    List of finite volume options

SourceFile
    optionList.C

\*---------------------------------------------------------------------------*/

#ifndef faOptionList_H
#define faOptionList_H

#include "faOption.H"
#include "PtrList.H"
#include "GeometricField.H"
#include "geometricOneField.H"
#include "faPatchField.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration of friend functions and operators
namespace fa
{
    class optionList;
}

Ostream& operator<<(Ostream& os, const fa::optionList& options);

namespace fa
{

/*---------------------------------------------------------------------------*\
                         Class optionList Declaration
\*---------------------------------------------------------------------------*/

class optionList
:
    public PtrList<option>
{
protected:

    // Protected Data

        //- Reference to the mesh database
        const fvMesh& mesh_;

        //- Reference to the patch
        const fvPatch& patch_;

        //- Time index to check that all defined sources have been applied
        label checkTimeIndex_;


    // Protected Member Functions

        //- Return the "options" sub-dictionary if present otherwise return dict
        const dictionary& optionsDict(const dictionary& dict) const;

        //- Read options dictionary
        bool readOptions(const dictionary& dict);

        //- Check that all sources have been applied
        void checkApplied() const;

        //- Return source for equation with specified name and dimensions
        template<class Type>
        tmp<faMatrix<Type>> source
        (
            GeometricField<Type, faPatchField, areaMesh>& field,
            const areaScalarField& h,
            const word& fieldName,
            const dimensionSet& ds
        );

        //- No copy construct
        optionList(const optionList&) = delete;

        //- No copy assignment
        void operator=(const optionList&) = delete;


public:

    //- Runtime type information
    TypeName("optionList");


    // Constructors

        //- Construct null
        optionList(const fvPatch& p);

        //- Construct from mesh and dictionary
        optionList(const fvPatch&, const dictionary& );


    //- Destructor
    virtual ~optionList()
    {}


    // Member Functions

        //- Reset the source list
        void reset(const dictionary& dict);


        // Sources

            //- Return source for equation
            template<class Type>
            tmp<faMatrix<Type>> operator()
            (
                const areaScalarField& h,
                GeometricField<Type, faPatchField, areaMesh>& field
            );

            //- Return source for equation with specified name
            template<class Type>
            tmp<faMatrix<Type>> operator()
            (
                const areaScalarField& h,
                GeometricField<Type, faPatchField, areaMesh>& field,
                const word& fieldName
            );

            //- Return source for equation
            template<class Type>
            tmp<faMatrix<Type>> operator()
            (
                const areaScalarField& h,
                const areaScalarField& rho,
                GeometricField<Type, faPatchField, areaMesh>& field
            );

            //- Return source for equation with specified name
            template<class Type>
            tmp<faMatrix<Type>> operator()
            (
                const areaScalarField& h,
                const areaScalarField& rho,
                GeometricField<Type, faPatchField, areaMesh>& field,
                const word& fieldName
            );


            //- Return source for equation with specified name and dimensios
            template<class Type>
            tmp<faMatrix<Type>> operator()
            (
                const areaScalarField& rho,
                GeometricField<Type, faPatchField, areaMesh>& field,
                const dimensionSet& ds
            );


            //- Return source for equation with second time derivative
            template<class Type>
            tmp<faMatrix<Type>> d2dt2
            (
                GeometricField<Type, faPatchField, areaMesh>& field
            );

            //- Return source for equation with second time derivative
            template<class Type>
            tmp<faMatrix<Type>> d2dt2
            (
                GeometricField<Type, faPatchField, areaMesh>& field,
                const word& fieldName
            );


        // Constraints

            //- Apply constraints to equation
            template<class Type>
            void constrain(faMatrix<Type>& eqn);


        // Correction

            //- Apply correction to field
            template<class Type>
            void correct(GeometricField<Type, faPatchField, areaMesh>& field);


        // IO

            //- Read dictionary
            virtual bool read(const dictionary& dict);

            //- Write data to Ostream
            virtual bool writeData(Ostream& os) const;

            //- Ostream operator
            friend Ostream& operator<<
            (
                Ostream& os,
                const optionList& options
            );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fa
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "faOptionListTemplates.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
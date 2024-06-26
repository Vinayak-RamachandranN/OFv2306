/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2018-2022 OpenCFD Ltd.
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

#include "pointZoneSet.H"
#include "mapPolyMesh.H"
#include "polyMesh.H"
#include "processorPolyPatch.H"
#include "cyclicPolyPatch.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(pointZoneSet, 0);
    addToRunTimeSelectionTable(topoSet, pointZoneSet, word);
    addToRunTimeSelectionTable(topoSet, pointZoneSet, size);
    addToRunTimeSelectionTable(topoSet, pointZoneSet, set);
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::pointZoneSet::updateSet()
{
    labelList order(sortedOrder(addressing_));
    inplaceReorder(order, addressing_);

    pointSet::clearStorage();
    pointSet::reserve(addressing_.size());
    pointSet::set(addressing_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pointZoneSet::pointZoneSet
(
    const polyMesh& mesh,
    const word& name,
    IOobjectOption::readOption rOpt,
    IOobjectOption::writeOption wOpt
)
:
    pointSet(mesh, name, 1024),  // do not read pointSet
    mesh_(mesh),
    addressing_()
{
    const pointZoneMesh& pointZones = mesh.pointZones();
    label zoneID = pointZones.findZoneID(name);

    if
    (
         IOobjectOption::isReadRequired(rOpt)
     || (IOobjectOption::isReadOptional(rOpt) && zoneID != -1)
    )
    {
        const pointZone& fz = pointZones[zoneID];
        addressing_ = fz;
    }

    updateSet();

    check(mesh.nPoints());
}


Foam::pointZoneSet::pointZoneSet
(
    const polyMesh& mesh,
    const word& name,
    const label size,
    IOobjectOption::writeOption wOpt
)
:
    pointSet(mesh, name, size, wOpt),
    mesh_(mesh),
    addressing_()
{
    updateSet();
}


Foam::pointZoneSet::pointZoneSet
(
    const polyMesh& mesh,
    const word& name,
    const topoSet& set,
    IOobjectOption::writeOption wOpt
)
:
    pointSet(mesh, name, set.size(), wOpt),
    mesh_(mesh),
    addressing_(refCast<const pointZoneSet>(set).addressing())
{
    updateSet();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::pointZoneSet::invert(const label maxLen)
{
    // Count
    label n = 0;

    for (label pointi = 0; pointi < maxLen; ++pointi)
    {
        if (!found(pointi))
        {
            ++n;
        }
    }

    // Fill
    addressing_.setSize(n);
    n = 0;

    for (label pointi = 0; pointi < maxLen; ++pointi)
    {
        if (!found(pointi))
        {
            addressing_[n] = pointi;
            ++n;
        }
    }
    updateSet();
}


void Foam::pointZoneSet::subset(const topoSet& set)
{
    DynamicList<label> newAddressing(addressing_.size());

    const pointZoneSet& zoneSet = refCast<const pointZoneSet>(set);

    for (const label pointi : zoneSet.addressing())
    {
        if (found(pointi))
        {
            newAddressing.append(pointi);
        }
    }

    addressing_.transfer(newAddressing);
    updateSet();
}


void Foam::pointZoneSet::addSet(const topoSet& set)
{
    DynamicList<label> newAddressing(addressing_);

    const pointZoneSet& zoneSet = refCast<const pointZoneSet>(set);

    for (const label pointi : zoneSet.addressing())
    {
        if (!found(pointi))
        {
            newAddressing.append(pointi);
        }
    }

    addressing_.transfer(newAddressing);
    updateSet();
}


void Foam::pointZoneSet::subtractSet(const topoSet& set)
{
    DynamicList<label> newAddressing(addressing_.size());

    const pointZoneSet& zoneSet = refCast<const pointZoneSet>(set);

    for (label pointi : addressing_)
    {
        if (!zoneSet.found(pointi))
        {
            // Not found in zoneSet so add
            newAddressing.append(pointi);
        }
    }

    addressing_.transfer(newAddressing);
    updateSet();
}


void Foam::pointZoneSet::sync(const polyMesh& mesh)
{
    pointSet::sync(mesh);

    // Take over contents of pointSet into addressing.
    addressing_ = sortedToc();
    updateSet();
}


Foam::label Foam::pointZoneSet::maxSize(const polyMesh& mesh) const
{
    return mesh.nPoints();
}


bool Foam::pointZoneSet::writeObject
(
    IOstreamOption streamOpt,
    const bool writeOnProc
) const
{
    // Write shadow pointSet
    word oldTypeName = typeName;
    const_cast<word&>(type()) = pointSet::typeName;
    bool ok = pointSet::writeObject(streamOpt, writeOnProc);
    const_cast<word&>(type()) = oldTypeName;

    // Modify pointZone
    pointZoneMesh& pointZones = const_cast<polyMesh&>(mesh_).pointZones();
    label zoneID = pointZones.findZoneID(name());

    if (zoneID == -1)
    {
        zoneID = pointZones.size();

        pointZones.emplace_back
        (
            name(),
            addressing_,
            zoneID,
            pointZones
        );
    }
    else
    {
        pointZones[zoneID] = addressing_;
    }
    pointZones.clearAddressing();

    return ok && pointZones.write(writeOnProc);
}


void Foam::pointZoneSet::updateMesh(const mapPolyMesh& morphMap)
{
    // pointZone
    labelList newAddressing(addressing_.size());

    label n = 0;
    for (const label pointi : addressing_)
    {
        const label newPointi = morphMap.reversePointMap()[pointi];
        if (newPointi >= 0)
        {
            newAddressing[n] = newPointi;
            ++n;
        }
    }
    newAddressing.resize(n);

    addressing_.transfer(newAddressing);

    updateSet();
}


void Foam::pointZoneSet::writeDebug
(
    Ostream& os,
    const primitiveMesh& mesh,
    const label maxLen
) const
{
    pointSet::writeDebug(os, mesh, maxLen);
}


// ************************************************************************* //

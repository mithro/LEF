// *****************************************************************************
// *****************************************************************************
// Copyright 2014, Cadence Design Systems
//
// This  file  is  part  of  the  Cadence  LEF/DEF  Open   Source
// Distribution,  Product Version 5.8.
//
// Licensed under the Apache License, Version 2.0 (the "License");
//    you may not use this file except in compliance with the License.
//    You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
//    Unless required by applicable law or agreed to in writing, software
//    distributed under the License is distributed on an "AS IS" BASIS,
//    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
//    implied. See the License for the specific language governing
//    permissions and limitations under the License.
//
// For updates, support, or to become part of the LEF/DEF Community,
// check www.openeda.org for details.
//
//  $Author$
//  $Revision$
//  $Date$
//  $State:  $
// *****************************************************************************
// *****************************************************************************

#ifdef WIN32
#pragma warning (disable : 4786)
#endif

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <stdlib.h>

#ifndef WIN32
#   include <unistd.h>
#else
#   include <windows.h>
#endif /* not WIN32 */
#include "lefrReader.hpp"
#include "lefwWriter.hpp"
#include "lefiDebug.hpp"
#include "lefiEncryptInt.hpp"
#include "lefiUtil.hpp"

#define FP "%.6f"
#define POINT FP " " FP
#define IN0 "  "
#define IN1 "    "
#define IN2 "      "
#define IN3 "        "

char defaultName[128];
char defaultOut[128];
FILE* fout;
int printing = 0;     // Printing the output.
int parse65nm = 0;
int parseLef58Type = 0;
int isSessionles = 0;

// TX_DIR:TRANSLATION ON

void dataError() {
  fprintf(fout, "ERROR: returned user data is not correct!\n");
}


void checkType(lefrCallbackType_e c) {
  if (c >= 0 && c <= lefrLibraryEndCbkType) {
    // OK
  } else {
    fprintf(fout, "ERROR: callback type is out of bounds!\n");
  }
}


char* orientStr(int orient) {
  switch (orient) {
      case 0: return ((char*)"N");
      case 1: return ((char*)"W");
      case 2: return ((char*)"S");
      case 3: return ((char*)"E");
      case 4: return ((char*)"FN");
      case 5: return ((char*)"FW");
      case 6: return ((char*)"FS");
      case 7: return ((char*)"FE");
  };
  return ((char*)"BOGUS");
}

void lefVia(lefiVia *via) {
    int i, j;

    lefrSetCaseSensitivity(1);
    fprintf(fout, "VIA %s ", via->lefiVia::name());
    if (via->lefiVia::hasDefault())
        fprintf(fout, "DEFAULT");
    else if (via->lefiVia::hasGenerated())
        fprintf(fout, "GENERATED");
    fprintf(fout, "\n");
    if (via->lefiVia::hasTopOfStack())
        fprintf(fout, IN0 "TOPOFSTACKONLY\n");
    if (via->lefiVia::hasForeign()) {
        fprintf(fout, IN0 "FOREIGN %s ", via->lefiVia::foreign());
        if (via->lefiVia::hasForeignPnt()) {
            fprintf(fout, "( " FP " " FP " ) ", via->lefiVia::foreignX(),
                    via->lefiVia::foreignY());
            if (via->lefiVia::hasForeignOrient())
                fprintf(fout, "%s ", orientStr(via->lefiVia::foreignOrient()));
        }
        fprintf(fout, ";\n");
    }
    if (via->lefiVia::hasProperties()) {
        fprintf(fout, IN0 "PROPERTY ");
        for (i = 0; i < via->lefiVia::numProperties(); i++) {
            fprintf(fout, "%s ", via->lefiVia::propName(i));
            if (via->lefiVia::propIsNumber(i))
                fprintf(fout, "" FP " ", via->lefiVia::propNumber(i));
            if (via->lefiVia::propIsString(i))
                fprintf(fout, "%s ", via->lefiVia::propValue(i));
            /*
            if (i+1 == via->lefiVia::numProperties())  // end of properties
            fprintf(fout, ";\n");
            else      // just add new line
            fprintf(fout, "\n");
            */
            switch (via->lefiVia::propType(i)) {
            case 'R':
                fprintf(fout, "REAL ");
                break;
            case 'I':
                fprintf(fout, "INTEGER ");
                break;
            case 'S':
                fprintf(fout, "STRING ");
                break;
            case 'Q':
                fprintf(fout, "QUOTESTRING ");
                break;
            case 'N':
                fprintf(fout, "NUMBER ");
                break;
            }
        }
        fprintf(fout, ";\n");
    }
    if (via->lefiVia::hasResistance())
        fprintf(fout, IN0 "RESISTANCE " FP " ;\n", via->lefiVia::resistance());
    if (via->lefiVia::numLayers() > 0) {
        for (i = 0; i < via->lefiVia::numLayers(); i++) {
            fprintf(fout, IN0 "LAYER %s\n", via->lefiVia::layerName(i));
            for (j = 0; j < via->lefiVia::numRects(i); j++)
                if (via->lefiVia::rectColorMask(i, j)) {
                    fprintf(fout, IN3 "RECT MASK %d ( " FP " " FP " ) ( " FP " " FP " ) ;\n",
                            via->lefiVia::rectColorMask(i, j),
                            via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                            via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
                } else {
                    fprintf(fout, IN3 "RECT " FP " " FP " " FP " " FP " ;\n",
                            via->lefiVia::xl(i, j), via->lefiVia::yl(i, j),
                            via->lefiVia::xh(i, j), via->lefiVia::yh(i, j));
                }
            for (j = 0; j < via->lefiVia::numPolygons(i); j++) {
                struct lefiGeomPolygon poly;
                poly = via->lefiVia::getPolygon(i, j);
                if (via->lefiVia::polyColorMask(i, j)) {
                    fprintf(fout, IN3 "POLYGON MASK %d", via->lefiVia::polyColorMask(i, j));
                } else {
                    fprintf(fout, IN3 "POLYGON ");
                }
                for (int k = 0; k < poly.numPoints; k++)
                    fprintf(fout, " " FP " " FP " ", poly.x[k], poly.y[k]);
                fprintf(fout, ";\n");
            }
        }
    }
    if (via->lefiVia::hasViaRule()) {
        fprintf(fout, IN0 "VIARULE %s ;\n", via->lefiVia::viaRuleName());
        fprintf(fout, IN1 "CUTSIZE " FP " " FP " ;\n", via->lefiVia::xCutSize(),
                via->lefiVia::yCutSize());
        fprintf(fout, IN1 "LAYERS %s %s %s ;\n", via->lefiVia::botMetalLayer(),
                via->lefiVia::cutLayer(), via->lefiVia::topMetalLayer());
        fprintf(fout, IN1 "CUTSPACING " FP " " FP " ;\n", via->lefiVia::xCutSpacing(),
                via->lefiVia::yCutSpacing());
        fprintf(fout, IN1 "ENCLOSURE " FP " " FP " " FP " " FP " ;\n", via->lefiVia::xBotEnc(),
                via->lefiVia::yBotEnc(), via->lefiVia::xTopEnc(),
                via->lefiVia::yTopEnc());
        if (via->lefiVia::hasRowCol())
            fprintf(fout, IN1 "ROWCOL %d %d ;\n", via->lefiVia::numCutRows(),
                    via->lefiVia::numCutCols());
        if (via->lefiVia::hasOrigin())
            fprintf(fout, IN1 "ORIGIN " FP " " FP " ;\n", via->lefiVia::xOffset(),
                    via->lefiVia::yOffset());
        if (via->lefiVia::hasOffset())
            fprintf(fout, IN1 "OFFSET " FP " " FP " " FP " " FP " ;\n", via->lefiVia::xBotOffset(),
                    via->lefiVia::yBotOffset(), via->lefiVia::xTopOffset(),
                    via->lefiVia::yTopOffset());
        if (via->lefiVia::hasCutPattern())
            fprintf(fout, IN1 "PATTERN %s ;\n", via->lefiVia::cutPattern());
    }
    fprintf(fout, "END %s\n", via->lefiVia::name());

    return;
}

void lefSpacing(lefiSpacing* spacing) {
  fprintf(fout, IN0 "SAMENET %s %s " FP " ", spacing->lefiSpacing::name1(),
          spacing->lefiSpacing::name2(), spacing->lefiSpacing::distance());
  if (spacing->lefiSpacing::hasStack())
     fprintf(fout, "STACK ");
  fprintf(fout,";\n");
  return;
}

void lefViaRuleLayer(lefiViaRuleLayer* vLayer) {
  fprintf(fout, IN0 "LAYER %s ;\n", vLayer->lefiViaRuleLayer::name());
  if (vLayer->lefiViaRuleLayer::hasDirection()) {
     if (vLayer->lefiViaRuleLayer::isHorizontal())
        fprintf(fout, IN1 "DIRECTION HORIZONTAL ;\n");
     if (vLayer->lefiViaRuleLayer::isVertical())
        fprintf(fout, IN1 "DIRECTION VERTICAL ;\n");
  }
  if (vLayer->lefiViaRuleLayer::hasEnclosure()) {
     fprintf(fout, IN1 "ENCLOSURE " FP " " FP " ;\n",
             vLayer->lefiViaRuleLayer::enclosureOverhang1(),
             vLayer->lefiViaRuleLayer::enclosureOverhang2());
  }
  if (vLayer->lefiViaRuleLayer::hasWidth())
     fprintf(fout, IN1 "WIDTH " FP " TO " FP " ;\n",
             vLayer->lefiViaRuleLayer::widthMin(),
             vLayer->lefiViaRuleLayer::widthMax());
  if (vLayer->lefiViaRuleLayer::hasResistance())
     fprintf(fout, IN1 "RESISTANCE " FP " ;\n",
             vLayer->lefiViaRuleLayer::resistance());
  if (vLayer->lefiViaRuleLayer::hasOverhang())
     fprintf(fout, IN1 "OVERHANG " FP " ;\n",
             vLayer->lefiViaRuleLayer::overhang());
  if (vLayer->lefiViaRuleLayer::hasMetalOverhang())
     fprintf(fout, IN1 "METALOVERHANG " FP " ;\n",
             vLayer->lefiViaRuleLayer::metalOverhang());
  if (vLayer->lefiViaRuleLayer::hasSpacing())
     fprintf(fout, IN1 "SPACING " FP " BY " FP " ;\n",
             vLayer->lefiViaRuleLayer::spacingStepX(),
             vLayer->lefiViaRuleLayer::spacingStepY());
  if (vLayer->lefiViaRuleLayer::hasRect())
     fprintf(fout, IN1 "RECT " FP " " FP " " FP " " FP " ;\n",
             vLayer->lefiViaRuleLayer::xl(), vLayer->lefiViaRuleLayer::yl(),
             vLayer->lefiViaRuleLayer::xh(), vLayer->lefiViaRuleLayer::yh());
  return;
}

void prtGeometry(lefiGeometries *geometry) {
    int                 numItems = geometry->lefiGeometries::numItems();
    int                 i, j;
    lefiGeomPath        *path;
    lefiGeomPathIter    *pathIter;
    lefiGeomRect        *rect;
    lefiGeomRectIter    *rectIter;
    lefiGeomPolygon     *polygon;
    lefiGeomPolygonIter *polygonIter;
    lefiGeomVia         *via;
    lefiGeomViaIter     *viaIter;

    for (i = 0; i < numItems; i++) {
        switch (geometry->lefiGeometries::itemType(i)) {
        case  lefiGeomClassE:
            fprintf(fout, "CLASS %s ",
                    geometry->lefiGeometries::getClass(i));
            break;
        case lefiGeomLayerE:
            fprintf(fout, IN2 "LAYER %s ;\n",
                    geometry->lefiGeometries::getLayer(i));
            break;
        case lefiGeomLayerExceptPgNetE:
            fprintf(fout, IN2 "EXCEPTPGNET ;\n");
            break;
        case lefiGeomLayerMinSpacingE:
            fprintf(fout, IN2 "SPACING " FP " ;\n",
                    geometry->lefiGeometries::getLayerMinSpacing(i));
            break;
        case lefiGeomLayerRuleWidthE:
            fprintf(fout, IN2 "DESIGNRULEWIDTH " FP " ;\n",
                    geometry->lefiGeometries::getLayerRuleWidth(i));
            break;
        case lefiGeomWidthE:
            fprintf(fout, IN2 "WIDTH " FP " ;\n",
                    geometry->lefiGeometries::getWidth(i));
            break;
        case lefiGeomPathE:
            path = geometry->lefiGeometries::getPath(i);
            if (path->colorMask != 0) {
                fprintf(fout, IN2 "PATH MASK %d ", path->colorMask);
            } else {
                fprintf(fout, IN2 "PATH ");
            }
            for (j = 0; j < path->numPoints; j++) {
                if (j + 1 == path->numPoints) // last one on the list
                    fprintf(fout, IN2 "( " FP " " FP " ) ;\n", path->x[j], path->y[j]);
                else
                    fprintf(fout, IN2 "( " FP " " FP " )\n", path->x[j], path->y[j]);
            }
            break;
        case lefiGeomPathIterE:
            pathIter = geometry->lefiGeometries::getPathIter(i);
            if (pathIter->colorMask != 0) {
                fprintf(fout, IN2 "PATH MASK %d ITERATED ", pathIter->colorMask);
            } else {
                fprintf(fout, IN2 "PATH ITERATED ");
            }
            for (j = 0; j < pathIter->numPoints; j++)
                fprintf(fout, IN2 "( " FP " " FP " )\n", pathIter->x[j],
                        pathIter->y[j]);
            fprintf(fout, IN2 "DO " FP " BY " FP " STEP " FP " " FP " ;\n", pathIter->xStart,
                    pathIter->yStart, pathIter->xStep, pathIter->yStep);
            break;
        case lefiGeomRectE:
            rect = geometry->lefiGeometries::getRect(i);
            if (rect->colorMask != 0) {
                fprintf(fout, IN3 "RECT MASK %d ( " FP " " FP " ) ( " FP " " FP " ) ;\n",
                        rect->colorMask, rect->xl,
                        rect->yl, rect->xh, rect->yh);
            } else {
                fprintf(fout, IN3 "RECT " FP " " FP " " FP " " FP " ;\n", rect->xl,
                        rect->yl, rect->xh, rect->yh);
            }
            break;
        case lefiGeomRectIterE:
            rectIter = geometry->lefiGeometries::getRectIter(i);
            if (rectIter->colorMask != 0) {
                fprintf(fout, IN2 "RECT MASK %d ITERATE ( " FP " " FP " ) ( " FP " " FP " )\n",
                        rectIter->colorMask,
                        rectIter->xl, rectIter->yl, rectIter->xh, rectIter->yh);
            } else {
                fprintf(fout, IN2 "RECT ITERATE ( " FP " " FP " ) ( " FP " " FP " )\n",
                        rectIter->xl, rectIter->yl, rectIter->xh, rectIter->yh);
            }
            fprintf(fout, IN2 "DO " FP " BY " FP " STEP " FP " " FP " ;\n",
                    rectIter->xStart, rectIter->yStart, rectIter->xStep,
                    rectIter->yStep);
            break;
        case lefiGeomPolygonE:
            polygon = geometry->lefiGeometries::getPolygon(i);
            if (polygon->colorMask != 0) {
                fprintf(fout, IN2 "POLYGON MASK %d ", polygon->colorMask);
            } else {
                fprintf(fout, IN2 "POLYGON ");
            }
            for (j = 0; j < polygon->numPoints; j++) {
                if (j + 1 == polygon->numPoints) // last one on the list
                    fprintf(fout, IN2 "( " FP " " FP " ) ;\n", polygon->x[j],
                            polygon->y[j]);
                else
                    fprintf(fout, IN2 "( " FP " " FP " )\n", polygon->x[j],
                            polygon->y[j]);
            }
            break;
        case lefiGeomPolygonIterE:
            polygonIter = geometry->lefiGeometries::getPolygonIter(i);
            if (polygonIter->colorMask != 0) {
                fprintf(fout, IN2 "POLYGON MASK %d ITERATE ", polygonIter->colorMask);
            } else {
                fprintf(fout, IN2 "POLYGON ITERATE");
            }
            for (j = 0; j < polygonIter->numPoints; j++)
                fprintf(fout, IN2 "( " FP " " FP " )\n", polygonIter->x[j],
                        polygonIter->y[j]);
            fprintf(fout, IN2 "DO " FP " BY " FP " STEP " FP " " FP " ;\n",
                    polygonIter->xStart, polygonIter->yStart,
                    polygonIter->xStep, polygonIter->yStep);
            break;
        case lefiGeomViaE:
            via = geometry->lefiGeometries::getVia(i);
            if (via->topMaskNum != 0 || via->bottomMaskNum != 0 || via->cutMaskNum !=0) {
                fprintf(fout, IN2 "VIA MASK %d%d%d ( " FP " " FP " ) %s ;\n",
                        via->topMaskNum, via->cutMaskNum, via->bottomMaskNum,
                        via->x, via->y,
                        via->name);

            } else {
                fprintf(fout, IN2 "VIA ( " FP " " FP " ) %s ;\n", via->x, via->y,
                        via->name);
            }
            break;
        case lefiGeomViaIterE:
            viaIter = geometry->lefiGeometries::getViaIter(i);
            if (viaIter->topMaskNum != 0 || viaIter->cutMaskNum != 0 || viaIter->bottomMaskNum != 0) {
                fprintf(fout, IN2 "VIA ITERATE MASK %d%d%d ( " FP " " FP " ) %s\n",
                        viaIter->topMaskNum, viaIter->cutMaskNum, viaIter->bottomMaskNum,
                        viaIter->x,
                        viaIter->y, viaIter->name);
            } else {
                fprintf(fout, IN2 "VIA ITERATE ( " FP " " FP " ) %s\n", viaIter->x,
                        viaIter->y, viaIter->name);
            }
            fprintf(fout, IN2 "DO " FP " BY " FP " STEP " FP " " FP " ;\n",
                    viaIter->xStart, viaIter->yStart,
                    viaIter->xStep, viaIter->yStep);
            break;
        default:
            fprintf(fout, "BOGUS geometries type.\n");
            break;
        }
    }
}

int antennaCB(lefrCallbackType_e c, double value, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  switch (c) {
        case lefrAntennaInputCbkType:
             fprintf(fout, "ANTENNAINPUTGATEAREA " FP " ;\n", value);
             break;
        case lefrAntennaInoutCbkType:
             fprintf(fout, "ANTENNAINOUTDIFFAREA " FP " ;\n", value);
             break;
        case lefrAntennaOutputCbkType:
             fprintf(fout, "ANTENNAOUTPUTDIFFAREA " FP " ;\n", value);
             break;
        case lefrInputAntennaCbkType:
             fprintf(fout, "INPUTPINANTENNASIZE " FP " ;\n", value);
             break;
        case lefrOutputAntennaCbkType:
             fprintf(fout, "OUTPUTPINANTENNASIZE " FP " ;\n", value);
             break;
        case lefrInoutAntennaCbkType:
             fprintf(fout, "INOUTPINANTENNASIZE " FP " ;\n", value);
             break;
        default:
             fprintf(fout, "BOGUS antenna type.\n");
             break;
  }
  return 0;
}

int arrayBeginCB(lefrCallbackType_e c, const char* name, lefiUserData ud) {
  int  status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwStartArray(name);
  if (status != LEFW_OK)
     return status;
  return 0;
}

int arrayCB(lefrCallbackType_e c, lefiArray* a, lefiUserData ud) {
  int              status, i, j, defCaps;
  lefiSitePattern* pattern;
  lefiTrackPattern* track;
  lefiGcellPattern* gcell;

  checkType(c);
  // if ((long)ud != userData) dataError();

  if (a->lefiArray::numSitePattern() > 0) {
     for (i = 0; i < a->lefiArray::numSitePattern(); i++) {
        pattern = a->lefiArray::sitePattern(i);
        status = lefwArraySite(pattern->lefiSitePattern::name(),
                               pattern->lefiSitePattern::x(),
                               pattern->lefiSitePattern::y(),
                               pattern->lefiSitePattern::orient(),
                               pattern->lefiSitePattern::xStart(),
                               pattern->lefiSitePattern::yStart(),
                               pattern->lefiSitePattern::xStep(),
                               pattern->lefiSitePattern::yStep());
        if (status != LEFW_OK)
           dataError();
     }
  }
  if (a->lefiArray::numCanPlace() > 0) {
     for (i = 0; i < a->lefiArray::numCanPlace(); i++) {
        pattern = a->lefiArray::canPlace(i);
        status = lefwArrayCanplace(pattern->lefiSitePattern::name(),
                                   pattern->lefiSitePattern::x(),
                                   pattern->lefiSitePattern::y(),
                                   pattern->lefiSitePattern::orient(),
                                   pattern->lefiSitePattern::xStart(),
                                   pattern->lefiSitePattern::yStart(),
                                   pattern->lefiSitePattern::xStep(),
                                   pattern->lefiSitePattern::yStep());
        if (status != LEFW_OK)
           dataError();
     }
  }
  if (a->lefiArray::numCannotOccupy() > 0) {
     for (i = 0; i < a->lefiArray::numCannotOccupy(); i++) {
        pattern = a->lefiArray::cannotOccupy(i);
        status = lefwArrayCannotoccupy(pattern->lefiSitePattern::name(),
                                       pattern->lefiSitePattern::x(),
                                       pattern->lefiSitePattern::y(),
                                       pattern->lefiSitePattern::orient(),
                                       pattern->lefiSitePattern::xStart(),
                                       pattern->lefiSitePattern::yStart(),
                                       pattern->lefiSitePattern::xStep(),
                                       pattern->lefiSitePattern::yStep());
        if (status != LEFW_OK)
           dataError();
     }
  }

  if (a->lefiArray::numTrack() > 0) {
     for (i = 0; i < a->lefiArray::numTrack(); i++) {
        track = a->lefiArray::track(i);
        fprintf(fout, IN0 "TRACKS %s, " FP " DO %d STEP " FP "\n",
                track->lefiTrackPattern::name(),
                track->lefiTrackPattern::start(),
                track->lefiTrackPattern::numTracks(),
                track->lefiTrackPattern::space());
        if (track->lefiTrackPattern::numLayers() > 0) {
           fprintf(fout, IN0 "LAYER ");
           for (j = 0; j < track->lefiTrackPattern::numLayers(); j++)
              fprintf(fout, "%s ", track->lefiTrackPattern::layerName(j));
           fprintf(fout, ";\n");
        }
     }
  }

  if (a->lefiArray::numGcell() > 0) {
     for (i = 0; i < a->lefiArray::numGcell(); i++) {
        gcell = a->lefiArray::gcell(i);
        fprintf(fout, IN0 "GCELLGRID %s, " FP " DO %d STEP " FP "\n",
                gcell->lefiGcellPattern::name(),
                gcell->lefiGcellPattern::start(),
                gcell->lefiGcellPattern::numCRs(),
                gcell->lefiGcellPattern::space());
     }
  }

  if (a->lefiArray::numFloorPlans() > 0) {
     for (i = 0; i < a->lefiArray::numFloorPlans(); i++) {
        status = lefwStartArrayFloorplan(a->lefiArray::floorPlanName(i));
        if (status != LEFW_OK)
           dataError();
        for (j = 0; j < a->lefiArray::numSites(i); j++) {
           pattern = a->lefiArray::site(i, j);
           status = lefwArrayFloorplan(a->lefiArray::siteType(i, j),
                                       pattern->lefiSitePattern::name(),
                                       pattern->lefiSitePattern::x(),
                                       pattern->lefiSitePattern::y(),
                                       pattern->lefiSitePattern::orient(),
                                       (int)pattern->lefiSitePattern::xStart(),
                                       (int)pattern->lefiSitePattern::yStart(),
                                       pattern->lefiSitePattern::xStep(),
                                       pattern->lefiSitePattern::yStep());
           if (status != LEFW_OK)
              dataError();
        }
     status = lefwEndArrayFloorplan(a->lefiArray::floorPlanName(i));
     if (status != LEFW_OK)
        dataError();
     }
  }

  defCaps = a->lefiArray::numDefaultCaps();
  if (defCaps > 0) {
     status = lefwStartArrayDefaultCap(defCaps);
     if (status != LEFW_OK)
        dataError();
     for (i = 0; i < defCaps; i++) {
        status = lefwArrayDefaultCap(a->lefiArray::defaultCapMinPins(i),
                                     a->lefiArray::defaultCap(i));
        if (status != LEFW_OK)
           dataError();
     }
     status = lefwEndArrayDefaultCap();
     if (status != LEFW_OK)
        dataError();
  }
  return 0;
}

int arrayEndCB(lefrCallbackType_e c, const char* name, lefiUserData ud) {
  int  status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write the data out
  status = lefwEndArray(name);
  if (status != LEFW_OK)
     return status;
  return 0;
}

int busBitCharsCB(lefrCallbackType_e c, const char* busBit, lefiUserData ud)
{
  int status;

  checkType(c);
  // if ((long)ud != userData) dataError();
  // use the lef writer to write out the data
  status = lefwBusBitChars(busBit);
  if (status != LEFW_OK)
     dataError();
  return 0;
}

int caseSensCB(lefrCallbackType_e c, int caseSense, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  if (caseSense == TRUE)
     fprintf(fout, "NAMESCASESENSITIVE ON ;\n");
  else
     fprintf(fout, "NAMESCASESENSITIVE OFF ;\n");
  return 0;
}

int fixedMaskCB(lefrCallbackType_e c, int fixedMask, lefiUserData ud) {
    checkType(c);

    if (fixedMask == 1)
        fprintf(fout, "FIXEDMASK ;\n");
    return 0;
}

int clearanceCB(lefrCallbackType_e c, const char* name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "CLEARANCEMEASURE %s ;\n", name);
  return 0;
}

int dividerCB(lefrCallbackType_e c, const char* name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "DIVIDER %s ;\n", name);
  return 0;
}

int noWireExtCB(lefrCallbackType_e c, const char* name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "NOWIREEXTENSION %s ;\n", name);
  return 0;
}

int noiseMarCB(lefrCallbackType_e c, lefiNoiseMargin *noise, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  return 0;
}

int edge1CB(lefrCallbackType_e c, double name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "EDGERATETHRESHOLD1 " FP " ;\n", name);
  return 0;
}

int edge2CB(lefrCallbackType_e c, double name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "EDGERATETHRESHOLD2 " FP " ;\n", name);
  return 0;
}

int edgeScaleCB(lefrCallbackType_e c, double name, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "EDGERATESCALEFACTORE " FP " ;\n", name);
  return 0;
}

int noiseTableCB(lefrCallbackType_e c, lefiNoiseTable *noise, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int correctionCB(lefrCallbackType_e c, lefiCorrectionTable *corr, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  return 0;
}

int dielectricCB(lefrCallbackType_e c, double dielectric, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();

  fprintf(fout, "DIELECTRIC " FP " ;\n", dielectric);
  return 0;
}

int irdropBeginCB(lefrCallbackType_e c, void* ptr, lefiUserData ud){
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "IRDROP\n");
  return 0;
}

int irdropCB(lefrCallbackType_e c, lefiIRDrop* irdrop, lefiUserData ud) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, IN0 "TABLE %s ", irdrop->lefiIRDrop::name());
  for (i = 0; i < irdrop->lefiIRDrop::numValues(); i++)
     fprintf(fout, "" FP " " FP " ", irdrop->lefiIRDrop::value1(i),
             irdrop->lefiIRDrop::value2(i));
  fprintf(fout, ";\n");
  return 0;
}

int irdropEndCB(lefrCallbackType_e c, void* ptr, lefiUserData ud){
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "END IRDROP\n");
  return 0;
}

int layerCB(lefrCallbackType_e c, lefiLayer* layer, lefiUserData ud) {
  int i, j, k;
  int numPoints, propNum;
  double *widths, *current;
  lefiLayerDensity* density;
  lefiAntennaPWL* pwl;
  lefiSpacingTable* spTable;
  lefiInfluence* influence;
  lefiParallel* parallel;
  lefiTwoWidths* twoWidths;
  char pType;
  int numMinCut, numMinenclosed;
  lefiAntennaModel* aModel;
  lefiOrthogonal*   ortho;

  checkType(c);
  // if ((long)ud != userData) dataError();

  lefrSetCaseSensitivity(0);

  // Call parse65nmRules for 5.7 syntax in 5.6
  if (parse65nm)
     layer->lefiLayer::parse65nmRules();

  // Call parseLef58Type for 5.8 syntax in 5.7
  if (parseLef58Type)
     layer->lefiLayer::parseLEF58Layer();

  fprintf(fout, "LAYER %s\n", layer->lefiLayer::name());
  if (layer->lefiLayer::hasType())
     fprintf(fout, IN0 "TYPE %s ;\n", layer->lefiLayer::type());
  if (layer->lefiLayer::hasLayerType())
     fprintf(fout, IN0 "LAYER TYPE %s ;\n", layer->lefiLayer::layerType());
  if (layer->lefiLayer::hasMask())
     fprintf(fout, IN0 "MASK %d ;\n", layer->lefiLayer::mask());
  if (layer->lefiLayer::hasPitch())
     fprintf(fout, IN0 "PITCH " FP " ;\n", layer->lefiLayer::pitch());
  else if (layer->lefiLayer::hasXYPitch())
     fprintf(fout, IN0 "PITCH " FP " " FP " ;\n", layer->lefiLayer::pitchX(),
             layer->lefiLayer::pitchY());
  if (layer->lefiLayer::hasOffset())
     fprintf(fout, IN0 "OFFSET " FP " ;\n", layer->lefiLayer::offset());
  else if (layer->lefiLayer::hasXYOffset())
     fprintf(fout, IN0 "OFFSET " FP " " FP " ;\n", layer->lefiLayer::offsetX(),
             layer->lefiLayer::offsetY());
  if (layer->lefiLayer::hasDiagPitch())
     fprintf(fout, IN0 "DIAGPITCH " FP " ;\n", layer->lefiLayer::diagPitch());
  else if (layer->lefiLayer::hasXYDiagPitch())
     fprintf(fout, IN0 "DIAGPITCH " FP " " FP " ;\n", layer->lefiLayer::diagPitchX(),
             layer->lefiLayer::diagPitchY());
  if (layer->lefiLayer::hasDiagWidth())
     fprintf(fout, IN0 "DIAGWIDTH " FP " ;\n", layer->lefiLayer::diagWidth());
  if (layer->lefiLayer::hasDiagSpacing())
     fprintf(fout, IN0 "DIAGSPACING " FP " ;\n", layer->lefiLayer::diagSpacing());
  if (layer->lefiLayer::hasWidth())
     fprintf(fout, IN0 "WIDTH " FP " ;\n", layer->lefiLayer::width());
  if (layer->lefiLayer::hasArea())
     fprintf(fout, IN0 "AREA " FP " ;\n", layer->lefiLayer::area());
  if (layer->lefiLayer::hasSlotWireWidth())
     fprintf(fout, IN0 "SLOTWIREWIDTH " FP " ;\n", layer->lefiLayer::slotWireWidth());
  if (layer->lefiLayer::hasSlotWireLength())
     fprintf(fout, IN0 "SLOTWIRELENGTH " FP " ;\n",
             layer->lefiLayer::slotWireLength());
  if (layer->lefiLayer::hasSlotWidth())
     fprintf(fout, IN0 "SLOTWIDTH " FP " ;\n", layer->lefiLayer::slotWidth());
  if (layer->lefiLayer::hasSlotLength())
     fprintf(fout, IN0 "SLOTLENGTH " FP " ;\n", layer->lefiLayer::slotLength());
  if (layer->lefiLayer::hasMaxAdjacentSlotSpacing())
     fprintf(fout, IN0 "MAXADJACENTSLOTSPACING " FP " ;\n",
             layer->lefiLayer::maxAdjacentSlotSpacing());
  if (layer->lefiLayer::hasMaxCoaxialSlotSpacing())
     fprintf(fout, IN0 "MAXCOAXIALSLOTSPACING " FP " ;\n",
             layer->lefiLayer::maxCoaxialSlotSpacing());
  if (layer->lefiLayer::hasMaxEdgeSlotSpacing())
     fprintf(fout, IN0 "MAXEDGESLOTSPACING " FP " ;\n",
             layer->lefiLayer::maxEdgeSlotSpacing());
  if (layer->lefiLayer::hasMaxFloatingArea())          // 5.7
     fprintf(fout, IN0 "MAXFLOATINGAREA " FP " ;\n",
             layer->lefiLayer::maxFloatingArea());
  if (layer->lefiLayer::hasArraySpacing()) {           // 5.7
     fprintf(fout, IN0 "ARRAYSPACING ");
     if (layer->lefiLayer::hasLongArray())
        fprintf(fout, "LONGARRAY ");
     if (layer->lefiLayer::hasViaWidth())
        fprintf(fout, "WIDTH " FP " ", layer->lefiLayer::viaWidth());
     fprintf(fout, "CUTSPACING " FP "", layer->lefiLayer::cutSpacing());
     for (i = 0; i < layer->lefiLayer::numArrayCuts(); i++)
        fprintf(fout, "\n\tARRAYCUTS %d SPACING " FP "",
                layer->lefiLayer::arrayCuts(i),
                layer->lefiLayer::arraySpacing(i));
     fprintf(fout, " ;\n");
  }
  if (layer->lefiLayer::hasSplitWireWidth())
     fprintf(fout, IN0 "SPLITWIREWIDTH " FP " ;\n",
             layer->lefiLayer::splitWireWidth());
  if (layer->lefiLayer::hasMinimumDensity())
     fprintf(fout, IN0 "MINIMUMDENSITY " FP " ;\n",
             layer->lefiLayer::minimumDensity());
  if (layer->lefiLayer::hasMaximumDensity())
     fprintf(fout, IN0 "MAXIMUMDENSITY " FP " ;\n",
             layer->lefiLayer::maximumDensity());
  if (layer->lefiLayer::hasDensityCheckWindow())
     fprintf(fout, IN0 "DENSITYCHECKWINDOW " FP " " FP " ;\n",
             layer->lefiLayer::densityCheckWindowLength(),
             layer->lefiLayer::densityCheckWindowWidth());
  if (layer->lefiLayer::hasDensityCheckStep())
     fprintf(fout, IN0 "DENSITYCHECKSTEP " FP " ;\n",
             layer->lefiLayer::densityCheckStep());
  if (layer->lefiLayer::hasFillActiveSpacing())
     fprintf(fout, IN0 "FILLACTIVESPACING " FP " ;\n",
             layer->lefiLayer::fillActiveSpacing());
  // 5.4.1
  numMinCut = layer->lefiLayer::numMinimumcut();
  if (numMinCut > 0) {
     for (i = 0; i < numMinCut; i++) {
         fprintf(fout, IN0 "MINIMUMCUT %d WIDTH " FP " ",
              layer->lefiLayer::minimumcut(i),
              layer->lefiLayer::minimumcutWidth(i));
         if (layer->lefiLayer::hasMinimumcutWithin(i))
            fprintf(fout, "WITHIN " FP " ", layer->lefiLayer::minimumcutWithin(i));
         if (layer->lefiLayer::hasMinimumcutConnection(i))
            fprintf(fout, "%s ", layer->lefiLayer::minimumcutConnection(i));
         if (layer->lefiLayer::hasMinimumcutNumCuts(i))
            fprintf(fout, "LENGTH " FP " WITHIN " FP " ",
            layer->lefiLayer::minimumcutLength(i),
            layer->lefiLayer::minimumcutDistance(i));
         fprintf(fout, ";\n");
     }
  }
  // 5.4.1
  if (layer->lefiLayer::hasMaxwidth()) {
     fprintf(fout, IN0 "MAXWIDTH " FP " ;\n", layer->lefiLayer::maxwidth());
  }
  // 5.5
  if (layer->lefiLayer::hasMinwidth()) {
     fprintf(fout, IN0 "MINWIDTH " FP " ;\n", layer->lefiLayer::minwidth());
  }
  // 5.5
  numMinenclosed = layer->lefiLayer::numMinenclosedarea();
  if (numMinenclosed > 0) {
     for (i = 0; i < numMinenclosed; i++) {
         fprintf(fout, IN0 "MINENCLOSEDAREA " FP " ",
              layer->lefiLayer::minenclosedarea(i));
         if (layer->lefiLayer::hasMinenclosedareaWidth(i))
              fprintf(fout, "MINENCLOSEDAREAWIDTH " FP " ",
                      layer->lefiLayer::minenclosedareaWidth(i));
         fprintf (fout, ";\n");
     }
  }
  // 5.4.1 & 5.6
  if (layer->lefiLayer::hasMinstep()) {
     for (i = 0; i < layer->lefiLayer::numMinstep(); i++) {
        fprintf(fout, IN0 "MINSTEP " FP " ", layer->lefiLayer::minstep(i));
        if (layer->lefiLayer::hasMinstepType(i))
           fprintf(fout, "%s ", layer->lefiLayer::minstepType(i));
        if (layer->lefiLayer::hasMinstepLengthsum(i))
           fprintf(fout, "LENGTHSUM " FP " ",
                   layer->lefiLayer::minstepLengthsum(i));
        if (layer->lefiLayer::hasMinstepMaxedges(i))
           fprintf(fout, "MAXEDGES %d ", layer->lefiLayer::minstepMaxedges(i));
        if (layer->lefiLayer::hasMinstepMinAdjLength(i))
           fprintf(fout, "MINADJLENGTH " FP " ", layer->lefiLayer::minstepMinAdjLength(i));
        if (layer->lefiLayer::hasMinstepMinBetLength(i))
           fprintf(fout, "MINBETLENGTH " FP " ", layer->lefiLayer::minstepMinBetLength(i));
        if (layer->lefiLayer::hasMinstepXSameCorners(i))
           fprintf(fout, "XSAMECORNERS");
        fprintf(fout, ";\n");
     }
  }
  // 5.4.1
  if (layer->lefiLayer::hasProtrusion()) {
     fprintf(fout, IN0 "PROTRUSIONWIDTH " FP " LENGTH " FP " WIDTH " FP " ;\n",
             layer->lefiLayer::protrusionWidth1(),
             layer->lefiLayer::protrusionLength(),
             layer->lefiLayer::protrusionWidth2());
  }
  if (layer->lefiLayer::hasSpacingNumber()) {
     for (i = 0; i < layer->lefiLayer::numSpacing(); i++) {
       fprintf(fout, IN0 "SPACING " FP " ", layer->lefiLayer::spacing(i));
       if (layer->lefiLayer::hasSpacingName(i))
          fprintf(fout, "LAYER %s ", layer->lefiLayer::spacingName(i));
       if (layer->lefiLayer::hasSpacingLayerStack(i))
          fprintf(fout, "STACK ");                           // 5.7
       if (layer->lefiLayer::hasSpacingAdjacent(i))
          fprintf(fout, "ADJACENTCUTS %d WITHIN " FP " ",
                  layer->lefiLayer::spacingAdjacentCuts(i),
                  layer->lefiLayer::spacingAdjacentWithin(i));
       if (layer->lefiLayer::hasSpacingAdjacentExcept(i))    // 5.7
          fprintf(fout, "EXCEPTSAMEPGNET ");
       if (layer->lefiLayer::hasSpacingCenterToCenter(i))
          fprintf(fout, "CENTERTOCENTER ");
       if (layer->lefiLayer::hasSpacingSamenet(i))           // 5.7
          fprintf(fout, "SAMENET ");
           if (layer->lefiLayer::hasSpacingSamenetPGonly(i)) // 5.7
              fprintf(fout, "PGONLY ");
       if (layer->lefiLayer::hasSpacingArea(i))              // 5.7
          fprintf(fout, "AREA " FP " ", layer->lefiLayer::spacingArea(i));
       if (layer->lefiLayer::hasSpacingRange(i)) {
          fprintf(fout, "RANGE " FP " " FP " ", layer->lefiLayer::spacingRangeMin(i),
                  layer->lefiLayer::spacingRangeMax(i));
          if (layer->lefiLayer::hasSpacingRangeUseLengthThreshold(i))
             fprintf(fout, "USELENGTHTHRESHOLD ");
          else if (layer->lefiLayer::hasSpacingRangeInfluence(i)) {
              fprintf(fout, "INFLUENCE " FP " ",
                 layer->lefiLayer::spacingRangeInfluence(i));
              if (layer->lefiLayer::hasSpacingRangeInfluenceRange(i))
                 fprintf(fout, "RANGE " FP " " FP " ",
                    layer->lefiLayer::spacingRangeInfluenceMin(i),
                    layer->lefiLayer::spacingRangeInfluenceMax(i));
           } else if (layer->lefiLayer::hasSpacingRangeRange(i))
               fprintf(fout, "RANGE " FP " " FP " ",
                 layer->lefiLayer::spacingRangeRangeMin(i),
                 layer->lefiLayer::spacingRangeRangeMax(i));
       } else if (layer->lefiLayer::hasSpacingLengthThreshold(i)) {
           fprintf(fout, "LENGTHTHRESHOLD " FP " ",
              layer->lefiLayer::spacingLengthThreshold(i));
           if (layer->lefiLayer::hasSpacingLengthThresholdRange(i))
              fprintf(fout, "RANGE " FP " " FP "",
                 layer->lefiLayer::spacingLengthThresholdRangeMin(i),
                 layer->lefiLayer::spacingLengthThresholdRangeMax(i));
       } else if (layer->lefiLayer::hasSpacingNotchLength(i)) {// 5.7
           fprintf(fout, "NOTCHLENGTH " FP "",
                   layer->lefiLayer::spacingNotchLength(i));
       } else if (layer->lefiLayer::hasSpacingEndOfNotchWidth(i)) // 5.7
           fprintf(fout, "ENDOFNOTCHWIDTH " FP " NOTCHSPACING " FP ", NOTCHLENGTH " FP "",
                   layer->lefiLayer::spacingEndOfNotchWidth(i),
                   layer->lefiLayer::spacingEndOfNotchSpacing(i),
                   layer->lefiLayer::spacingEndOfNotchLength(i));

       if (layer->lefiLayer::hasSpacingParallelOverlap(i))   // 5.7
          fprintf(fout, "PARALLELOVERLAP ");
       if (layer->lefiLayer::hasSpacingEndOfLine(i)) {       // 5.7
          fprintf(fout, "ENDOFLINE " FP " WITHIN " FP " ",
             layer->lefiLayer::spacingEolWidth(i),
             layer->lefiLayer::spacingEolWithin(i));
          if (layer->lefiLayer::hasSpacingParellelEdge(i)) {
             fprintf(fout, "PARALLELEDGE " FP " WITHIN " FP " ",
                layer->lefiLayer::spacingParSpace(i),
                layer->lefiLayer::spacingParWithin(i));
             if (layer->lefiLayer::hasSpacingTwoEdges(i)) {
                fprintf(fout, "TWOEDGES ");
             }
          }
       }
       fprintf(fout, ";\n");
     }
  }
  if (layer->lefiLayer::hasSpacingTableOrtho()) {            // 5.7
     fprintf(fout, "SPACINGTABLE ORTHOGONAL");
     ortho = layer->lefiLayer::orthogonal();
     for (i = 0; i < ortho->lefiOrthogonal::numOrthogonal(); i++) {
        fprintf(fout, "\n   WITHIN " FP " SPACING " FP "",
                ortho->lefiOrthogonal::cutWithin(i),
                ortho->lefiOrthogonal::orthoSpacing(i));
     }
     fprintf(fout, ";\n");
  }
  for (i = 0; i < layer->lefiLayer::numEnclosure(); i++) {
     fprintf(fout, "ENCLOSURE ");
     if (layer->lefiLayer::hasEnclosureRule(i))
        fprintf(fout, "%s ", layer->lefiLayer::enclosureRule(i));
     fprintf(fout, "" FP " " FP " ", layer->lefiLayer::enclosureOverhang1(i),
                             layer->lefiLayer::enclosureOverhang2(i));
     if (layer->lefiLayer::hasEnclosureWidth(i))
        fprintf(fout, "WIDTH " FP " ", layer->lefiLayer::enclosureMinWidth(i));
     if (layer->lefiLayer::hasEnclosureExceptExtraCut(i))
        fprintf(fout, "EXCEPTEXTRACUT " FP " ",
                layer->lefiLayer::enclosureExceptExtraCut(i));
     if (layer->lefiLayer::hasEnclosureMinLength(i))
        fprintf(fout, "LENGTH " FP " ", layer->lefiLayer::enclosureMinLength(i));
     fprintf(fout, ";\n");
  }
  for (i = 0; i < layer->lefiLayer::numPreferEnclosure(); i++) {
     fprintf(fout, "PREFERENCLOSURE ");
     if (layer->lefiLayer::hasPreferEnclosureRule(i))
        fprintf(fout, "%s ", layer->lefiLayer::preferEnclosureRule(i));
     fprintf(fout, "" FP " " FP " ", layer->lefiLayer::preferEnclosureOverhang1(i),
                             layer->lefiLayer::preferEnclosureOverhang2(i));
     if (layer->lefiLayer::hasPreferEnclosureWidth(i))
        fprintf(fout, "WIDTH " FP " ",layer->lefiLayer::preferEnclosureMinWidth(i));
     fprintf(fout, ";\n");
  }
  if (layer->lefiLayer::hasResistancePerCut())
     fprintf(fout, IN0 "RESISTANCE " FP " ;\n",
             layer->lefiLayer::resistancePerCut());
  if (layer->lefiLayer::hasCurrentDensityPoint())
     fprintf(fout, IN0 "CURRENTDEN " FP " ;\n",
             layer->lefiLayer::currentDensityPoint());
  if (layer->lefiLayer::hasCurrentDensityArray()) {
     layer->lefiLayer::currentDensityArray(&numPoints, &widths, &current);
     for (i = 0; i < numPoints; i++)
         fprintf(fout, IN0 "CURRENTDEN ( " FP " " FP " ) ;\n", widths[i], current[i]);
  }
  if (layer->lefiLayer::hasDirection())
     fprintf(fout, IN0 "DIRECTION %s ;\n", layer->lefiLayer::direction());
  if (layer->lefiLayer::hasResistance())
     fprintf(fout, IN0 "RESISTANCE RPERSQ " FP " ;\n",
             layer->lefiLayer::resistance());
  if (layer->lefiLayer::hasCapacitance())
     fprintf(fout, IN0 "CAPACITANCE CPERSQDIST " FP " ;\n",
             layer->lefiLayer::capacitance());
  if (layer->lefiLayer::hasEdgeCap())
     fprintf(fout, IN0 "EDGECAPACITANCE " FP " ;\n", layer->lefiLayer::edgeCap());
  if (layer->lefiLayer::hasHeight())
     fprintf(fout, IN0 "TYPE " FP " ;\n", layer->lefiLayer::height());
  if (layer->lefiLayer::hasThickness())
     fprintf(fout, IN0 "THICKNESS " FP " ;\n", layer->lefiLayer::thickness());
  if (layer->lefiLayer::hasWireExtension())
     fprintf(fout, IN0 "WIREEXTENSION " FP " ;\n", layer->lefiLayer::wireExtension());
  if (layer->lefiLayer::hasShrinkage())
     fprintf(fout, IN0 "SHRINKAGE " FP " ;\n", layer->lefiLayer::shrinkage());
  if (layer->lefiLayer::hasCapMultiplier())
     fprintf(fout, IN0 "CAPMULTIPLIER " FP " ;\n", layer->lefiLayer::capMultiplier());
  if (layer->lefiLayer::hasAntennaArea())
     fprintf(fout, IN0 "ANTENNAAREAFACTOR " FP " ;\n",
             layer->lefiLayer::antennaArea());
  if (layer->lefiLayer::hasAntennaLength())
     fprintf(fout, IN0 "ANTENNALENGTHFACTOR " FP " ;\n",
             layer->lefiLayer::antennaLength());

  // 5.5 AntennaModel
  for (i = 0; i < layer->lefiLayer::numAntennaModel(); i++) {
     aModel = layer->lefiLayer::antennaModel(i);

     fprintf(fout, IN0 "ANTENNAMODEL %s ;\n",
             aModel->lefiAntennaModel::antennaOxide());

     if (aModel->lefiAntennaModel::hasAntennaAreaRatio())
        fprintf(fout, IN0 "ANTENNAAREARATIO " FP " ;\n",
                aModel->lefiAntennaModel::antennaAreaRatio());
     if (aModel->lefiAntennaModel::hasAntennaDiffAreaRatio())
        fprintf(fout, IN0 "ANTENNADIFFAREARATIO " FP " ;\n",
                aModel->lefiAntennaModel::antennaDiffAreaRatio());
     else if (aModel->lefiAntennaModel::hasAntennaDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffAreaRatioPWL();
        fprintf(fout, IN0 "ANTENNADIFFAREARATIO PWL ( ");
        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
           fprintf(fout, "( " FP " " FP " ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
                   pwl->lefiAntennaPWL::PWLratio(j));
        fprintf(fout, ") ;\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaCumAreaRatio())
        fprintf(fout, IN0 "ANTENNACUMAREARATIO " FP " ;\n",
                aModel->lefiAntennaModel::antennaCumAreaRatio());
     if (aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatio())
        fprintf(fout, IN0 "ANTENNACUMDIFFAREARATIO " FP "\n",
                aModel->lefiAntennaModel::antennaCumDiffAreaRatio());
     if (aModel->lefiAntennaModel::hasAntennaCumDiffAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffAreaRatioPWL();
        fprintf(fout, IN0 "ANTENNACUMDIFFAREARATIO PWL ( ");
        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
           fprintf(fout, "( " FP " " FP " ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
                   pwl->lefiAntennaPWL::PWLratio(j));
        fprintf(fout, ") ;\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaAreaFactor()) {
        fprintf(fout, IN0 "ANTENNAAREAFACTOR " FP " ",
                aModel->lefiAntennaModel::antennaAreaFactor());
        if (aModel->lefiAntennaModel::hasAntennaAreaFactorDUO())
           fprintf(fout, IN0 "DIFFUSEONLY ");
        fprintf(fout, ";\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaSideAreaRatio())
        fprintf(fout, IN0 "ANTENNASIDEAREARATIO " FP " ;\n",
                aModel->lefiAntennaModel::antennaSideAreaRatio());
     if (aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatio())
        fprintf(fout, IN0 "ANTENNADIFFSIDEAREARATIO " FP "\n",
                aModel->lefiAntennaModel::antennaDiffSideAreaRatio());
     else if (aModel->lefiAntennaModel::hasAntennaDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaDiffSideAreaRatioPWL();
        fprintf(fout, IN0 "ANTENNADIFFSIDEAREARATIO PWL ( ");
        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
           fprintf(fout, "( " FP " " FP " ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
                   pwl->lefiAntennaPWL::PWLratio(j));
        fprintf(fout, ") ;\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaCumSideAreaRatio())
        fprintf(fout, IN0 "ANTENNACUMSIDEAREARATIO " FP " ;\n",
                aModel->lefiAntennaModel::antennaCumSideAreaRatio());
     if (aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatio())
        fprintf(fout, IN0 "ANTENNACUMDIFFSIDEAREARATIO " FP "\n",
                aModel->lefiAntennaModel::antennaCumDiffSideAreaRatio());
     else if (aModel->lefiAntennaModel::hasAntennaCumDiffSideAreaRatioPWL()) {
        pwl = aModel->lefiAntennaModel::antennaCumDiffSideAreaRatioPWL();
        fprintf(fout, IN0 "ANTENNACUMDIFFSIDEAREARATIO PWL ( ");
        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
           fprintf(fout, "( " FP " " FP " ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
                   pwl->lefiAntennaPWL::PWLratio(j));
        fprintf(fout, ") ;\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaSideAreaFactor()) {
        fprintf(fout, IN0 "ANTENNASIDEAREAFACTOR " FP " ",
                aModel->lefiAntennaModel::antennaSideAreaFactor());
        if (aModel->lefiAntennaModel::hasAntennaSideAreaFactorDUO())
           fprintf(fout, IN0 "DIFFUSEONLY ");
        fprintf(fout, ";\n");
     }
     if (aModel->lefiAntennaModel::hasAntennaCumRoutingPlusCut())
        fprintf(fout, IN0 "ANTENNACUMROUTINGPLUSCUT ;\n");
     if (aModel->lefiAntennaModel::hasAntennaGatePlusDiff())
        fprintf(fout, IN0 "ANTENNAGATEPLUSDIFF " FP " ;\n",
                aModel->lefiAntennaModel::antennaGatePlusDiff());
     if (aModel->lefiAntennaModel::hasAntennaAreaMinusDiff())
        fprintf(fout, IN0 "ANTENNAAREAMINUSDIFF " FP " ;\n",
                aModel->lefiAntennaModel::antennaAreaMinusDiff());
     if (aModel->lefiAntennaModel::hasAntennaAreaDiffReducePWL()) {
        pwl = aModel->lefiAntennaModel::antennaAreaDiffReducePWL();
        fprintf(fout, IN0 "ANTENNAAREADIFFREDUCEPWL ( ");
        for (j = 0; j < pwl->lefiAntennaPWL::numPWL(); j++)
           fprintf(fout, "( " FP " " FP " ) ", pwl->lefiAntennaPWL::PWLdiffusion(j),
                   pwl->lefiAntennaPWL::PWLratio(j));
        fprintf(fout, ") ;\n");
     }
  }

  if (layer->lefiLayer::numAccurrentDensity()) {
     for (i = 0; i < layer->lefiLayer::numAccurrentDensity(); i++) {
         density = layer->lefiLayer::accurrent(i);
         fprintf(fout, IN0 "ACCURRENTDENSITY %s", density->type());
         if (density->hasOneEntry())
             fprintf(fout, " " FP " ;\n", density->oneEntry());
         else {
             fprintf(fout, "\n");
             if (density->numFrequency()) {
                fprintf(fout, IN1 "FREQUENCY");
                for (j = 0; j < density->numFrequency(); j++)
                   fprintf(fout, " " FP "", density->frequency(j));
                fprintf(fout, " ;\n");
             }
             if (density->numCutareas()) {
                fprintf(fout, IN1 "CUTAREA");
                for (j = 0; j < density->numCutareas(); j++)
                   fprintf(fout, " " FP "", density->cutArea(j));
                fprintf(fout, " ;\n");
             }
             if (density->numWidths()) {
                fprintf(fout, IN1 "WIDTH");
                for (j = 0; j < density->numWidths(); j++)
                   fprintf(fout, " " FP "", density->width(j));
                fprintf(fout, " ;\n");
             }
             if (density->numTableEntries()) {
                k = 5;
                fprintf(fout, IN1 "TABLEENTRIES");
                for (j = 0; j < density->numTableEntries(); j++)
                   if (k > 4) {
                      fprintf(fout, "\n     " FP "", density->tableEntry(j));
                      k = 1;
                   } else {
                      fprintf(fout, " " FP "", density->tableEntry(j));
                      k++;
                   }
                fprintf(fout, " ;\n");
             }
         }
     }
  }
  if (layer->lefiLayer::numDccurrentDensity()) {
     for (i = 0; i < layer->lefiLayer::numDccurrentDensity(); i++) {
         density = layer->lefiLayer::dccurrent(i);
         fprintf(fout, IN0 "DCCURRENTDENSITY %s", density->type());
         if (density->hasOneEntry())
             fprintf(fout, " " FP " ;\n", density->oneEntry());
         else {
             fprintf(fout, "\n");
             if (density->numCutareas()) {
                fprintf(fout, IN1 "CUTAREA");
                for (j = 0; j < density->numCutareas(); j++)
                   fprintf(fout, " " FP "", density->cutArea(j));
                fprintf(fout, " ;\n");
             }
             if (density->numWidths()) {
                fprintf(fout, IN1 "WIDTH");
                for (j = 0; j < density->numWidths(); j++)
                   fprintf(fout, " " FP "", density->width(j));
                fprintf(fout, " ;\n");
             }
             if (density->numTableEntries()) {
                fprintf(fout, IN1 "TABLEENTRIES");
                for (j = 0; j < density->numTableEntries(); j++)
                   fprintf(fout, " " FP "", density->tableEntry(j));
                fprintf(fout, " ;\n");
             }
         }
     }
  }

  for (i = 0; i < layer->lefiLayer::numSpacingTable(); i++) {
     spTable = layer->lefiLayer::spacingTable(i);
     fprintf(fout, "   SPACINGTABLE\n");
     if (spTable->lefiSpacingTable::isInfluence()) {
        influence = spTable->lefiSpacingTable::influence();
        fprintf(fout, IN2 "INFLUENCE");
        for (j = 0; j < influence->lefiInfluence::numInfluenceEntry(); j++) {
           fprintf(fout, "\n          WIDTH " FP " WITHIN " FP " SPACING " FP "",
                   influence->lefiInfluence::width(j),
                   influence->lefiInfluence::distance(j),
                   influence->lefiInfluence::spacing(j));
        }
        fprintf(fout, " ;\n");
     } else if (spTable->lefiSpacingTable::isParallel()){
        parallel = spTable->lefiSpacingTable::parallel();
        fprintf(fout, IN2 "PARALLELRUNLENGTH");
        for (j = 0; j < parallel->lefiParallel::numLength(); j++) {
           fprintf(fout, " " FP "", parallel->lefiParallel::length(j));
        }
        for (j = 0; j < parallel->lefiParallel::numWidth(); j++) {
           fprintf(fout, "\n          WIDTH " FP "",
                   parallel->lefiParallel::width(j));
           for (k = 0; k < parallel->lefiParallel::numLength(); k++) {
              fprintf(fout, " " FP "", parallel->lefiParallel::widthSpacing(j, k));
           }
        }
        fprintf(fout, " ;\n");
     } else {    // 5.7 TWOWIDTHS
        twoWidths = spTable->lefiSpacingTable::twoWidths();
        fprintf(fout, IN2 "TWOWIDTHS");
        for (j = 0; j < twoWidths->lefiTwoWidths::numWidth(); j++) {
           fprintf(fout, "\n          WIDTH " FP " ",
                   twoWidths->lefiTwoWidths::width(j));
           if (twoWidths->lefiTwoWidths::hasWidthPRL(j))
              fprintf(fout, "PRL " FP " ", twoWidths->lefiTwoWidths::widthPRL(j));
           for (k = 0; k < twoWidths->lefiTwoWidths::numWidthSpacing(j); k++)
              fprintf(fout, "" FP " ",twoWidths->lefiTwoWidths::widthSpacing(j, k));
        }
        fprintf(fout, " ;\n");
     }
  }

  propNum = layer->lefiLayer::numProps();
  if (propNum > 0) {
     fprintf(fout, IN0 "PROPERTY ");
     for (i = 0; i < propNum; i++) {
        // value can either be a string or number
        fprintf(fout, "%s ", layer->lefiLayer::propName(i));
        if (layer->lefiLayer::propIsNumber(i))
            fprintf(fout, "" FP " ", layer->lefiLayer::propNumber(i));
        if (layer->lefiLayer::propIsString(i))
            fprintf(fout, "%s ", layer->lefiLayer::propValue(i));
        pType = layer->lefiLayer::propType(i);
        switch (pType) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
     }
     fprintf(fout, ";\n");
  }
  if (layer->lefiLayer::hasDiagMinEdgeLength())
     fprintf(fout, IN0 "DIAGMINEDGELENGTH " FP " ;\n",
             layer->lefiLayer::diagMinEdgeLength());
  if (layer->lefiLayer::numMinSize()) {
     fprintf(fout, IN0 "MINSIZE ");
     for (i = 0; i < layer->lefiLayer::numMinSize(); i++) {
        fprintf(fout, "" FP " " FP " ", layer->lefiLayer::minSizeWidth(i),
                                layer->lefiLayer::minSizeLength(i));
     }
     fprintf(fout, ";\n");
  }

  fprintf(fout, "END %s\n", layer->lefiLayer::name());

  // Set it to case sensitive from here on
  lefrSetCaseSensitivity(1);

  return 0;
}

int macroBeginCB(lefrCallbackType_e c, const char* macroName, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "MACRO %s\n",  macroName);
  return 0;
}

int macroFixedMaskCB(lefrCallbackType_e c, int fixedMask,
                     lefiUserData ud) {
  checkType(c);

  return 0;
}

int macroClassTypeCB(lefrCallbackType_e c, const char* macroClassType,
                     lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "MACRO CLASS %s\n",  macroClassType);
  return 0;
}

int macroOriginCB(lefrCallbackType_e c, lefiNum macroNum,
                     lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // fprintf(fout, IN0 "ORIGIN ( " FP " " FP " ) ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroSizeCB(lefrCallbackType_e c, lefiNum macroNum,
                     lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  // fprintf(fout, IN0 "SIZE " FP " BY " FP " ;\n", macroNum.x, macroNum.y);
  return 0;
}

int macroCB(lefrCallbackType_e c, lefiMacro* macro, lefiUserData ud) {
  lefiSitePattern* pattern;
  int              propNum, i, hasPrtSym = 0;

  checkType(c);
  // if ((long)ud != userData) dataError();
  if (macro->lefiMacro::hasClass())
     fprintf(fout, IN0 "CLASS %s ;\n", macro->lefiMacro::macroClass());
  if (macro->lefiMacro::isFixedMask())
      fprintf(fout, IN0 "FIXEDMASK ;\n");
  if (macro->lefiMacro::hasEEQ())
     fprintf(fout, IN0 "EEQ %s ;\n", macro->lefiMacro::EEQ());
  if (macro->lefiMacro::hasLEQ())
     fprintf(fout, IN0 "LEQ %s ;\n", macro->lefiMacro::LEQ());
  if (macro->lefiMacro::hasSource())
     fprintf(fout, IN0 "SOURCE %s ;\n", macro->lefiMacro::source());
  if (macro->lefiMacro::hasXSymmetry()) {
     fprintf(fout, IN0 "SYMMETRY X ");
     hasPrtSym = 1;
  }
  if (macro->lefiMacro::hasYSymmetry()) {   // print X Y & R90 in one line
     if (!hasPrtSym) {
        fprintf(fout, IN0 "SYMMETRY Y ");
        hasPrtSym = 1;
     }
     else
        fprintf(fout, "Y ");
  }
  if (macro->lefiMacro::has90Symmetry()) {
     if (!hasPrtSym) {
        fprintf(fout, IN0 "SYMMETRY R90 ");
        hasPrtSym = 1;
     }
     else
        fprintf(fout, "R90 ");
  }
  if (hasPrtSym) {
     fprintf (fout, ";\n");
     hasPrtSym = 0;
  }
  if (macro->lefiMacro::hasSiteName())
     fprintf(fout, IN0 "SITE %s ;\n", macro->lefiMacro::siteName());
  if (macro->lefiMacro::hasSitePattern()) {
     for (i = 0; i < macro->lefiMacro::numSitePattern(); i++ ) {
       pattern = macro->lefiMacro::sitePattern(i);
       if (pattern->lefiSitePattern::hasStepPattern()) {
          fprintf(fout, IN0 "SITE %s " FP " " FP " %s DO " FP " BY " FP " STEP " FP " " FP " ;\n",
                pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
                pattern->lefiSitePattern::y(),
                orientStr(pattern->lefiSitePattern::orient()),
                pattern->lefiSitePattern::xStart(),
                pattern->lefiSitePattern::yStart(),
                pattern->lefiSitePattern::xStep(),
                pattern->lefiSitePattern::yStep());
       } else {
          fprintf(fout, IN0 "SITE %s " FP " " FP " %s ;\n",
                pattern->lefiSitePattern::name(), pattern->lefiSitePattern::x(),
                pattern->lefiSitePattern::y(),
                orientStr(pattern->lefiSitePattern::orient()));
       }
     }
  }
  if (macro->lefiMacro::hasSize())
     fprintf(fout, IN0 "SIZE " FP " BY " FP " ;\n", macro->lefiMacro::sizeX(),
             macro->lefiMacro::sizeY());

  if (macro->lefiMacro::hasForeign()) {
     for (i = 0; i < macro->lefiMacro::numForeigns(); i++) {
        fprintf(fout, IN0 "FOREIGN %s ", macro->lefiMacro::foreignName(i));
        if (macro->lefiMacro::hasForeignPoint(i)) {
           fprintf(fout, "( " FP " " FP " ) ", macro->lefiMacro::foreignX(i),
                   macro->lefiMacro::foreignY(i));
           if (macro->lefiMacro::hasForeignOrient(i))
              fprintf(fout, "%s ", macro->lefiMacro::foreignOrientStr(i));
        }
        fprintf(fout, ";\n");
     }
  }
  if (macro->lefiMacro::hasOrigin())
     fprintf(fout, IN0 "ORIGIN " FP " " FP " ;\n", macro->lefiMacro::originX(),
             macro->lefiMacro::originY());
  if (macro->lefiMacro::hasPower())
     fprintf(fout, IN0 "POWER " FP " ;\n", macro->lefiMacro::power());
  propNum = macro->lefiMacro::numProperties();
  if (propNum > 0) {
     fprintf(fout, IN0 "PROPERTY ");
     for (i = 0; i < propNum; i++) {
        // value can either be a string or number
        if (macro->lefiMacro::propValue(i)) {
           fprintf(fout, "%s %s ", macro->lefiMacro::propName(i),
                   macro->lefiMacro::propValue(i));
        }
        else
           fprintf(fout, "%s " FP " ", macro->lefiMacro::propName(i),
                   macro->lefiMacro::propNum(i));

        switch (macro->lefiMacro::propType(i)) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
     }
     fprintf(fout, ";\n");
  }
  //fprintf(fout, "END %s\n", macro->lefiMacro::name());
  return 0;
}

int macroEndCB(lefrCallbackType_e c, const char* macroName, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "END %s\n", macroName);
  return 0;
}

int manufacturingCB(lefrCallbackType_e c, double num, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "MANUFACTURINGGRID " FP " ;\n", num);
  return 0;
}

int maxStackViaCB(lefrCallbackType_e c, lefiMaxStackVia* maxStack,
  lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "MAXVIASTACK %d ", maxStack->lefiMaxStackVia::maxStackVia());
  if (maxStack->lefiMaxStackVia::hasMaxStackViaRange())
     fprintf(fout, "RANGE %s %s ",
             maxStack->lefiMaxStackVia::maxStackViaBottomLayer(),
             maxStack->lefiMaxStackVia::maxStackViaTopLayer());
  fprintf(fout, ";\n");
  return 0;
}

int minFeatureCB(lefrCallbackType_e c, lefiMinFeature* min, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "MINFEATURE " FP " " FP " ;\n", min->lefiMinFeature::one(),
          min->lefiMinFeature::two());
  return 0;
}

int nonDefaultCB(lefrCallbackType_e c, lefiNonDefault* def, lefiUserData ud) {
  int          i;
  lefiVia*     via;
  lefiSpacing* spacing;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "NONDEFAULTRULE %s\n", def->lefiNonDefault::name());
  if (def->lefiNonDefault::hasHardspacing())
     fprintf(fout, IN0 "HARDSPACING ;\n");
  for (i = 0; i < def->lefiNonDefault::numLayers(); i++) {
     fprintf(fout, IN0 "LAYER %s\n", def->lefiNonDefault::layerName(i));
     if (def->lefiNonDefault::hasLayerWidth(i))
        fprintf(fout, IN1 "WIDTH " FP " ;\n", def->lefiNonDefault::layerWidth(i));
     if (def->lefiNonDefault::hasLayerSpacing(i))
        fprintf(fout, IN1 "SPACING " FP " ;\n",
                def->lefiNonDefault::layerSpacing(i));
     if (def->lefiNonDefault::hasLayerDiagWidth(i))
        fprintf(fout, IN1 "DIAGWIDTH " FP " ;\n",
                def->lefiNonDefault::layerDiagWidth(i));
     if (def->lefiNonDefault::hasLayerWireExtension(i))
        fprintf(fout, IN1 "WIREEXTENSION " FP " ;\n",
                def->lefiNonDefault::layerWireExtension(i));
     if (def->lefiNonDefault::hasLayerResistance(i))
        fprintf(fout, IN1 "RESISTANCE RPERSQ " FP " ;\n",
                def->lefiNonDefault::layerResistance(i));
     if (def->lefiNonDefault::hasLayerCapacitance(i))
        fprintf(fout, IN1 "CAPACITANCE CPERSQDIST " FP " ;\n",
                def->lefiNonDefault::layerCapacitance(i));
     if (def->lefiNonDefault::hasLayerEdgeCap(i))
        fprintf(fout, IN1 "EDGECAPACITANCE " FP " ;\n",
                def->lefiNonDefault::layerEdgeCap(i));
     fprintf(fout, IN0 "END %s\n", def->lefiNonDefault::layerName(i));
  }

  // handle via in nondefaultrule
  for (i = 0; i < def->lefiNonDefault::numVias(); i++) {
     via = def->lefiNonDefault::viaRule(i);
     lefVia(via);
  }

  // handle spacing in nondefaultrule
  for (i = 0; i < def->lefiNonDefault::numSpacingRules(); i++) {
     spacing = def->lefiNonDefault::spacingRule(i);
     lefSpacing(spacing);
  }

  // handle usevia
  for (i = 0; i < def->lefiNonDefault::numUseVia(); i++)
     fprintf(fout, IN1 "USEVIA %s ;\n", def->lefiNonDefault::viaName(i));

  // handle useviarule
  for (i = 0; i < def->lefiNonDefault::numUseViaRule(); i++)
     fprintf(fout, IN1 "USEVIARULE %s ;\n",
             def->lefiNonDefault::viaRuleName(i));

  // handle mincuts
  for (i = 0; i < def->lefiNonDefault::numMinCuts(); i++) {
     fprintf(fout, "   MINCUTS %s %d ;\n", def->lefiNonDefault::cutLayerName(i),
             def->lefiNonDefault::numCuts(i));
  }

  // handle property in nondefaultrule
  if (def->lefiNonDefault::numProps() > 0) {
     fprintf(fout, "   PROPERTY ");
     for (i = 0; i < def->lefiNonDefault::numProps(); i++) {
        fprintf(fout, "%s ", def->lefiNonDefault::propName(i));
        if (def->lefiNonDefault::propIsNumber(i))
            fprintf(fout, "" FP " ", def->lefiNonDefault::propNumber(i));
        if (def->lefiNonDefault::propIsString(i))
            fprintf(fout, "%s ", def->lefiNonDefault::propValue(i));
        switch(def->lefiNonDefault::propType(i)) {
            case 'R': fprintf(fout, "REAL ");
                      break;
            case 'I': fprintf(fout, "INTEGER ");
                      break;
            case 'S': fprintf(fout, "STRING ");
                      break;
            case 'Q': fprintf(fout, "QUOTESTRING ");
                      break;
            case 'N': fprintf(fout, "NUMBER ");
                      break;
        }
     }
     fprintf(fout, ";\n");
  }
  fprintf(fout, "END %s ;\n", def->lefiNonDefault::name());

  return 0;
}

int obstructionCB(lefrCallbackType_e c, lefiObstruction* obs,
                  lefiUserData ud) {
  lefiGeometries* geometry;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, IN0 "OBS\n");
  geometry = obs->lefiObstruction::geometries();
  prtGeometry(geometry);
  fprintf(fout, IN0 "END\n");
  return 0;
}

int pinCB(lefrCallbackType_e c, lefiPin* pin, lefiUserData ud) {
  int                  numPorts, i, j;
  lefiGeometries*      geometry;
  lefiPinAntennaModel* aModel;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, IN0 "PIN %s\n", pin->lefiPin::name());
  if (pin->lefiPin::hasForeign()) {
     for (i = 0; i < pin->lefiPin::numForeigns(); i++) {
        if (pin->lefiPin::hasForeignOrient(i))
           fprintf(fout, IN1 "FOREIGN %s STRUCTURE ( " FP " " FP " ) %s ;\n",
                   pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                   pin->lefiPin::foreignY(i),
                   pin->lefiPin::foreignOrientStr(i));
        else if (pin->lefiPin::hasForeignPoint(i))
           fprintf(fout, IN1 "FOREIGN %s STRUCTURE ( " FP " " FP " ) ;\n",
                   pin->lefiPin::foreignName(i), pin->lefiPin::foreignX(i),
                   pin->lefiPin::foreignY(i));
        else
           fprintf(fout, IN1 "FOREIGN %s ;\n", pin->lefiPin::foreignName(i));
     }
  }
  if (pin->lefiPin::hasLEQ())
     fprintf(fout, IN1 "LEQ %s ;\n", pin->lefiPin::LEQ());
  if (pin->lefiPin::hasDirection())
     fprintf(fout, IN1 "DIRECTION %s ;\n", pin->lefiPin::direction());
  if (pin->lefiPin::hasUse())
     fprintf(fout, IN1 "USE %s ;\n", pin->lefiPin::use());
  if (pin->lefiPin::hasShape())
     fprintf(fout, IN1 "SHAPE %s ;\n", pin->lefiPin::shape());
  if (pin->lefiPin::hasMustjoin())
     fprintf(fout, IN1 "MUSTJOIN %s ;\n", pin->lefiPin::mustjoin());
  if (pin->lefiPin::hasOutMargin())
     fprintf(fout, IN1 "OUTPUTNOISEMARGIN " FP " " FP " ;\n",
             pin->lefiPin::outMarginHigh(), pin->lefiPin::outMarginLow());
  if (pin->lefiPin::hasOutResistance())
     fprintf(fout, IN1 "OUTPUTRESISTANCE " FP " " FP " ;\n",
             pin->lefiPin::outResistanceHigh(),
             pin->lefiPin::outResistanceLow());
  if (pin->lefiPin::hasInMargin())
     fprintf(fout, IN1 "INPUTNOISEMARGIN " FP " " FP " ;\n",
             pin->lefiPin::inMarginHigh(), pin->lefiPin::inMarginLow());
  if (pin->lefiPin::hasPower())
     fprintf(fout, IN1 "POWER " FP " ;\n", pin->lefiPin::power());
  if (pin->lefiPin::hasLeakage())
     fprintf(fout, IN1 "LEAKAGE " FP " ;\n", pin->lefiPin::leakage());
  if (pin->lefiPin::hasMaxload())
     fprintf(fout, IN1 "MAXLOAD " FP " ;\n", pin->lefiPin::maxload());
  if (pin->lefiPin::hasCapacitance())
     fprintf(fout, IN1 "CAPACITANCE " FP " ;\n", pin->lefiPin::capacitance());
  if (pin->lefiPin::hasResistance())
     fprintf(fout, IN1 "RESISTANCE " FP " ;\n", pin->lefiPin::resistance());
  if (pin->lefiPin::hasPulldownres())
     fprintf(fout, IN1 "PULLDOWNRES " FP " ;\n", pin->lefiPin::pulldownres());
  if (pin->lefiPin::hasTieoffr())
     fprintf(fout, IN1 "TIEOFFR " FP " ;\n", pin->lefiPin::tieoffr());
  if (pin->lefiPin::hasVHI())
     fprintf(fout, IN1 "VHI " FP " ;\n", pin->lefiPin::VHI());
  if (pin->lefiPin::hasVLO())
     fprintf(fout, IN1 "VLO " FP " ;\n", pin->lefiPin::VLO());
  if (pin->lefiPin::hasRiseVoltage())
     fprintf(fout, IN1 "RISEVOLTAGETHRESHOLD " FP " ;\n",
             pin->lefiPin::riseVoltage());
  if (pin->lefiPin::hasFallVoltage())
     fprintf(fout, IN1 "FALLVOLTAGETHRESHOLD " FP " ;\n",
             pin->lefiPin::fallVoltage());
  if (pin->lefiPin::hasRiseThresh())
     fprintf(fout, IN1 "RISETHRESH " FP " ;\n", pin->lefiPin::riseThresh());
  if (pin->lefiPin::hasFallThresh())
     fprintf(fout, IN1 "FALLTHRESH " FP " ;\n", pin->lefiPin::fallThresh());
  if (pin->lefiPin::hasRiseSatcur())
     fprintf(fout, IN1 "RISESATCUR " FP " ;\n", pin->lefiPin::riseSatcur());
  if (pin->lefiPin::hasFallSatcur())
     fprintf(fout, IN1 "FALLSATCUR " FP " ;\n", pin->lefiPin::fallSatcur());
  if (pin->lefiPin::hasRiseSlewLimit())
     fprintf(fout, IN1 "RISESLEWLIMIT " FP " ;\n", pin->lefiPin::riseSlewLimit());
  if (pin->lefiPin::hasFallSlewLimit())
     fprintf(fout, IN1 "FALLSLEWLIMIT " FP " ;\n", pin->lefiPin::fallSlewLimit());
  if (pin->lefiPin::hasCurrentSource())
     fprintf(fout, IN1 "CURRENTSOURCE %s ;\n", pin->lefiPin::currentSource());
  if (pin->lefiPin::hasTables())
     fprintf(fout, IN1 "IV_TABLES %s %s ;\n", pin->lefiPin::tableHighName(),
             pin->lefiPin::tableLowName());
  if (pin->lefiPin::hasTaperRule())
     fprintf(fout, IN1 "TAPERRULE %s ;\n", pin->lefiPin::taperRule());
  if (pin->lefiPin::hasNetExpr())
     fprintf(fout, IN1 "NETEXPR \"%s\" ;\n", pin->lefiPin::netExpr());
  if (pin->lefiPin::hasSupplySensitivity())
     fprintf(fout, IN1 "SUPPLYSENSITIVITY %s ;\n",
             pin->lefiPin::supplySensitivity());
  if (pin->lefiPin::hasGroundSensitivity())
     fprintf(fout, IN1 "GROUNDSENSITIVITY %s ;\n",
             pin->lefiPin::groundSensitivity());
  if (pin->lefiPin::hasAntennaSize()) {
     for (i = 0; i < pin->lefiPin::numAntennaSize(); i++) {
        fprintf(fout, IN1 "ANTENNASIZE " FP " ", pin->lefiPin::antennaSize(i));
        if (pin->lefiPin::antennaSizeLayer(i))
           fprintf(fout, "LAYER %s ", pin->lefiPin::antennaSizeLayer(i));
        fprintf(fout, ";\n");
     }
  }
  if (pin->lefiPin::hasAntennaMetalArea()) {
     for (i = 0; i < pin->lefiPin::numAntennaMetalArea(); i++) {
        fprintf(fout, IN1 "ANTENNAMETALAREA " FP " ",
           pin->lefiPin::antennaMetalArea(i));
        if (pin->lefiPin::antennaMetalAreaLayer(i))
           fprintf(fout, "LAYER %s ", pin->lefiPin::antennaMetalAreaLayer(i));
        fprintf(fout, ";\n");
     }
  }
  if (pin->lefiPin::hasAntennaMetalLength()) {
     for (i = 0; i < pin->lefiPin::numAntennaMetalLength(); i++) {
        fprintf(fout, IN1 "ANTENNAMETALLENGTH " FP " ",
           pin->lefiPin::antennaMetalLength(i));
        if (pin->lefiPin::antennaMetalLengthLayer(i))
           fprintf(fout, "LAYER %s ", pin->lefiPin::antennaMetalLengthLayer(i));
        fprintf(fout, ";\n");
     }
  }

  if (pin->lefiPin::hasAntennaPartialMetalArea()) {
     for (i = 0; i < pin->lefiPin::numAntennaPartialMetalArea(); i++) {
        fprintf(fout, IN1 "ANTENNAPARTIALMETALAREA " FP " ",
                pin->lefiPin::antennaPartialMetalArea(i));
        if (pin->lefiPin::antennaPartialMetalAreaLayer(i))
           fprintf(fout, "LAYER %s ",
                   pin->lefiPin::antennaPartialMetalAreaLayer(i));
        fprintf(fout, ";\n");
     }
  }

  if (pin->lefiPin::hasAntennaPartialMetalSideArea()) {
     for (i = 0; i < pin->lefiPin::numAntennaPartialMetalSideArea(); i++) {
        fprintf(fout, IN1 "ANTENNAPARTIALMETALSIDEAREA " FP " ",
                pin->lefiPin::antennaPartialMetalSideArea(i));
        if (pin->lefiPin::antennaPartialMetalSideAreaLayer(i))
           fprintf(fout, "LAYER %s ",
                   pin->lefiPin::antennaPartialMetalSideAreaLayer(i));
        fprintf(fout, ";\n");
     }
  }

  if (pin->lefiPin::hasAntennaPartialCutArea()) {
     for (i = 0; i < pin->lefiPin::numAntennaPartialCutArea(); i++) {
        fprintf(fout, IN1 "ANTENNAPARTIALCUTAREA " FP " ",
                pin->lefiPin::antennaPartialCutArea(i));
        if (pin->lefiPin::antennaPartialCutAreaLayer(i))
           fprintf(fout, "LAYER %s ",
                   pin->lefiPin::antennaPartialCutAreaLayer(i));
        fprintf(fout, ";\n");
     }
  }

  if (pin->lefiPin::hasAntennaDiffArea()) {
     for (i = 0; i < pin->lefiPin::numAntennaDiffArea(); i++) {
        fprintf(fout, IN1 "ANTENNADIFFAREA " FP " ",
                pin->lefiPin::antennaDiffArea(i));
        if (pin->lefiPin::antennaDiffAreaLayer(i))
           fprintf(fout, "LAYER %s ", pin->lefiPin::antennaDiffAreaLayer(i));
        fprintf(fout, ";\n");
     }
  }

  for (j = 0; j < pin->lefiPin::numAntennaModel(); j++) {
     aModel = pin->lefiPin::antennaModel(j);

     fprintf(fout, IN1 "ANTENNAMODEL %s ;\n",
             aModel->lefiPinAntennaModel::antennaOxide());

     if (aModel->lefiPinAntennaModel::hasAntennaGateArea()) {
        for (i = 0; i < aModel->lefiPinAntennaModel::numAntennaGateArea(); i++)
        {
           fprintf(fout, IN1 "ANTENNAGATEAREA " FP " ",
                   aModel->lefiPinAntennaModel::antennaGateArea(i));
           if (aModel->lefiPinAntennaModel::antennaGateAreaLayer(i))
              fprintf(fout, "LAYER %s ",
                      aModel->lefiPinAntennaModel::antennaGateAreaLayer(i));
           fprintf(fout, ";\n");
        }
     }

     if (aModel->lefiPinAntennaModel::hasAntennaMaxAreaCar()) {
        for (i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxAreaCar();
           i++) {
           fprintf(fout, IN1 "ANTENNAMAXAREACAR " FP " ",
                   aModel->lefiPinAntennaModel::antennaMaxAreaCar(i));
           if (aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i))
              fprintf(fout, "LAYER %s ",
                   aModel->lefiPinAntennaModel::antennaMaxAreaCarLayer(i));
           fprintf(fout, ";\n");
        }
     }

     if (aModel->lefiPinAntennaModel::hasAntennaMaxSideAreaCar()) {
        for (i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxSideAreaCar();
           i++) {
           fprintf(fout, IN1 "ANTENNAMAXSIDEAREACAR " FP " ",
                   aModel->lefiPinAntennaModel::antennaMaxSideAreaCar(i));
           if (aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i))
              fprintf(fout, "LAYER %s ",
                   aModel->lefiPinAntennaModel::antennaMaxSideAreaCarLayer(i));
           fprintf(fout, ";\n");
        }
     }

     if (aModel->lefiPinAntennaModel::hasAntennaMaxCutCar()) {
        for (i = 0; i < aModel->lefiPinAntennaModel::numAntennaMaxCutCar(); i++)
        {
           fprintf(fout, IN1 "ANTENNAMAXCUTCAR " FP " ",
                   aModel->lefiPinAntennaModel::antennaMaxCutCar(i));
           if (aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i))
              fprintf(fout, "LAYER %s ",
                   aModel->lefiPinAntennaModel::antennaMaxCutCarLayer(i));
           fprintf(fout, ";\n");
        }
     }
  }

  if (pin->lefiPin::numProperties() > 0) {
     fprintf(fout, IN1 "PROPERTY ");
     for (i = 0; i < pin->lefiPin::numProperties(); i++) {
        // value can either be a string or number
        if (pin->lefiPin::propValue(i)) {
           fprintf(fout, "%s %s ", pin->lefiPin::propName(i),
                   pin->lefiPin::propValue(i));
        }
        else
           fprintf(fout, "%s " FP " ", pin->lefiPin::propName(i),
                   pin->lefiPin::propNum(i));
        switch (pin->lefiPin::propType(i)) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
     }
     fprintf(fout, ";\n");
  }

  numPorts = pin->lefiPin::numPorts();
  for (i = 0; i < numPorts; i++) {
     fprintf(fout,IN1 "PORT\n");
     geometry = pin->lefiPin::port(i);
     prtGeometry(geometry);
     fprintf(fout, IN1 "END\n");
  }
  fprintf(fout, IN0 "END %s\n", pin->lefiPin::name());
  return 0;
}

int densityCB(lefrCallbackType_e c, lefiDensity* density,
                  lefiUserData ud) {

  struct lefiGeomRect rect;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, IN0 "DENSITY\n");
  for (int i = 0; i < density->lefiDensity::numLayer(); i++) {
    fprintf(fout, IN1 "LAYER %s ;\n", density->lefiDensity::layerName(i));
    for (int j = 0; j < density->lefiDensity::numRects(i); j++) {
      rect = density->lefiDensity::getRect(i,j);
      fprintf(fout, IN2 "RECT " FP " " FP " " FP " " FP " ", rect.xl, rect.yl, rect.xh,
              rect.yh);
      fprintf(fout, "" FP " ;\n", density->lefiDensity::densityValue(i,j));
    }
  }
  fprintf(fout, IN0 "END\n");
  return 0;
}

int propDefBeginCB(lefrCallbackType_e c, void* ptr, lefiUserData ud) {

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "PROPERTYDEFINITIONS\n");
  return 0;
}

int propDefCB(lefrCallbackType_e c, lefiProp* prop, lefiUserData ud) {

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, " %s %s", prop->lefiProp::propType(),
          prop->lefiProp::propName());
  switch(prop->lefiProp::dataType()) {
     case 'I':
          fprintf(fout, " INTEGER");
          break;
     case 'R':
          fprintf(fout, " REAL");
          break;
     case 'S':
          fprintf(fout, " STRING");
          break;
  }
  if (prop->lefiProp::hasNumber())
     fprintf(fout, " " FP "", prop->lefiProp::number());
  if (prop->lefiProp::hasRange())
     fprintf(fout, " RANGE " FP " " FP "", prop->lefiProp::left(),
             prop->lefiProp::right());
  if (prop->lefiProp::hasString())
     fprintf(fout, " %s", prop->lefiProp::string());
  fprintf(fout, "\n");
  return 0;
}

int propDefEndCB(lefrCallbackType_e c, void* ptr, lefiUserData ud) {

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "END PROPERTYDEFINITIONS\n");
  return 0;
}

int siteCB(lefrCallbackType_e c, lefiSite* site, lefiUserData ud) {
  int hasPrtSym = 0;
  int i;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "SITE %s\n", site->lefiSite::name());
  if (site->lefiSite::hasClass())
     fprintf(fout, IN0 "CLASS %s ;\n", site->lefiSite::siteClass());
  if (site->lefiSite::hasXSymmetry()) {
     fprintf(fout, IN0 "SYMMETRY X ");
     hasPrtSym = 1;
  }
  if (site->lefiSite::hasYSymmetry()) {
     if (hasPrtSym)
        fprintf(fout, "Y ");
     else {
        fprintf(fout, IN0 "SYMMETRY Y ");
        hasPrtSym = 1;
     }
  }
  if (site->lefiSite::has90Symmetry()) {
     if (hasPrtSym)
        fprintf(fout, "R90 ");
     else {
        fprintf(fout, IN0 "SYMMETRY R90 ");
        hasPrtSym = 1;
     }
  }
  if (hasPrtSym)
     fprintf(fout, ";\n");
  if (site->lefiSite::hasSize())
     fprintf(fout, IN0 "SIZE " FP " BY " FP " ;\n", site->lefiSite::sizeX(),
             site->lefiSite::sizeY());

  if (site->hasRowPattern()) {
     fprintf(fout, IN0 "ROWPATTERN ");
     for (i = 0; i < site->lefiSite::numSites(); i++)
        fprintf(fout, IN0 "%s %s ", site->lefiSite::siteName(i),
                site->lefiSite::siteOrientStr(i));
     fprintf(fout, ";\n");
  }

  fprintf(fout, "END %s\n", site->lefiSite::name());
  return 0;
}

int spacingBeginCB(lefrCallbackType_e c, void* ptr, lefiUserData ud){
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "SPACING\n");
  return 0;
}

int spacingCB(lefrCallbackType_e c, lefiSpacing* spacing, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  lefSpacing(spacing);
  return 0;
}

int spacingEndCB(lefrCallbackType_e c, void* ptr, lefiUserData ud){
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "END SPACING\n");
  return 0;
}

int timingCB(lefrCallbackType_e c, lefiTiming* timing, lefiUserData ud) {
  int i;
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "TIMING\n");
  for (i = 0; i < timing->numFromPins(); i++)
     fprintf(fout, " FROMPIN %s ;\n", timing->fromPin(i));
  for (i = 0; i < timing->numToPins(); i++)
     fprintf(fout, " TOPIN %s ;\n", timing->toPin(i));
     fprintf(fout, " RISE SLEW1 " FP " " FP " " FP " " FP " ;\n", timing->riseSlewOne(),
             timing->riseSlewTwo(), timing->riseSlewThree(),
             timing->riseSlewFour());
  if (timing->hasRiseSlew2())
     fprintf(fout, " RISE SLEW2 " FP " " FP " " FP " ;\n", timing->riseSlewFive(),
             timing->riseSlewSix(), timing->riseSlewSeven());
  if (timing->hasFallSlew())
     fprintf(fout, " FALL SLEW1 " FP " " FP " " FP " " FP " ;\n", timing->fallSlewOne(),
             timing->fallSlewTwo(), timing->fallSlewThree(),
             timing->fallSlewFour());
  if (timing->hasFallSlew2())
     fprintf(fout, " FALL SLEW2 " FP " " FP " " FP " ;\n", timing->fallSlewFive(),
             timing->fallSlewSix(), timing->riseSlewSeven());
  if (timing->hasRiseIntrinsic()) {
     fprintf(fout, "TIMING RISE INTRINSIC " FP " " FP " ;\n",
             timing->riseIntrinsicOne(), timing->riseIntrinsicTwo());
     fprintf(fout, "TIMING RISE VARIABLE " FP " " FP " ;\n",
             timing->riseIntrinsicThree(), timing->riseIntrinsicFour());
  }
  if (timing->hasFallIntrinsic()) {
     fprintf(fout, "TIMING FALL INTRINSIC " FP " " FP " ;\n",
             timing->fallIntrinsicOne(), timing->fallIntrinsicTwo());
     fprintf(fout, "TIMING RISE VARIABLE " FP " " FP " ;\n",
             timing->fallIntrinsicThree(), timing->fallIntrinsicFour());
  }
  if (timing->hasRiseRS())
     fprintf(fout, "TIMING RISERS " FP " " FP " ;\n",
             timing->riseRSOne(), timing->riseRSTwo());
     if (timing->hasRiseCS())
     fprintf(fout, "TIMING RISECS " FP " " FP " ;\n",
             timing->riseCSOne(), timing->riseCSTwo());
  if (timing->hasFallRS())
     fprintf(fout, "TIMING FALLRS " FP " " FP " ;\n",
             timing->fallRSOne(), timing->fallRSTwo());
  if (timing->hasFallCS())
     fprintf(fout, "TIMING FALLCS " FP " " FP " ;\n",
             timing->fallCSOne(), timing->fallCSTwo());
  if (timing->hasUnateness())
     fprintf(fout, "TIMING UNATENESS %s ;\n", timing->unateness());
  if (timing->hasRiseAtt1())
     fprintf(fout, "TIMING RISESATT1 " FP " " FP " ;\n", timing->riseAtt1One(),
             timing->riseAtt1Two());
  if (timing->hasFallAtt1())
     fprintf(fout, "TIMING FALLSATT1 " FP " " FP " ;\n", timing->fallAtt1One(),
             timing->fallAtt1Two());
  if (timing->hasRiseTo())
     fprintf(fout, "TIMING RISET0 " FP " " FP " ;\n", timing->riseToOne(),
             timing->riseToTwo());
  if (timing->hasFallTo())
     fprintf(fout, "TIMING FALLT0 " FP " " FP " ;\n", timing->fallToOne(),
             timing->fallToTwo());
  if (timing->hasSDFonePinTrigger())
     fprintf(fout, " %s TABLEDIMENSION " FP " " FP " " FP " ;\n",
             timing->SDFonePinTriggerType(), timing->SDFtriggerOne(),
             timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  if (timing->hasSDFtwoPinTrigger())
     fprintf(fout, " %s %s %s TABLEDIMENSION " FP " " FP " " FP " ;\n",
             timing->SDFtwoPinTriggerType(), timing->SDFfromTrigger(),
             timing->SDFtoTrigger(), timing->SDFtriggerOne(),
             timing->SDFtriggerTwo(), timing->SDFtriggerThree());
  fprintf(fout, "END TIMING\n");
  return 0;
}

int unitsCB(lefrCallbackType_e c, lefiUnits* unit, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "UNITS\n");
  if (unit->lefiUnits::hasDatabase())
     fprintf(fout, IN0 "DATABASE %s " FP " ;\n", unit->lefiUnits::databaseName(),
             unit->lefiUnits::databaseNumber());
  if (unit->lefiUnits::hasCapacitance())
     fprintf(fout, IN0 "CAPACITANCE PICOFARADS " FP " ;\n",
             unit->lefiUnits::capacitance());
  if (unit->lefiUnits::hasResistance())
     fprintf(fout, IN0 "RESISTANCE OHMS " FP " ;\n", unit->lefiUnits::resistance());
  if (unit->lefiUnits::hasPower())
     fprintf(fout, IN0 "POWER MILLIWATTS " FP " ;\n", unit->lefiUnits::power());
  if (unit->lefiUnits::hasCurrent())
     fprintf(fout, IN0 "CURRENT MILLIAMPS " FP " ;\n", unit->lefiUnits::current());
  if (unit->lefiUnits::hasVoltage())
     fprintf(fout, IN0 "VOLTAGE VOLTS " FP " ;\n", unit->lefiUnits::voltage());
  if (unit->lefiUnits::hasFrequency())
     fprintf(fout, IN0 "FREQUENCY MEGAHERTZ " FP " ;\n",
             unit->lefiUnits::frequency());
  fprintf(fout, "END UNITS\n");
  return 0;
}

int useMinSpacingCB(lefrCallbackType_e c, lefiUseMinSpacing* spacing,
                    lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "USEMINSPACING %s ", spacing->lefiUseMinSpacing::name());
  if (spacing->lefiUseMinSpacing::value())
      fprintf(fout, "ON ;\n");
  else
      fprintf(fout, "OFF ;\n");
  return 0;
}

int versionCB(lefrCallbackType_e c, double num, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "VERSION %.1f ;\n", num);
  return 0;
}

int versionStrCB(lefrCallbackType_e c, const char* versionName, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "VERSION %s ;\n", versionName);
  return 0;
}

int viaCB(lefrCallbackType_e c, lefiVia* via, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  lefVia(via);
  return 0;
}

int viaRuleCB(lefrCallbackType_e c, lefiViaRule* viaRule, lefiUserData ud) {
  int               numLayers, numVias, i;
  lefiViaRuleLayer* vLayer;

  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "VIARULE %s", viaRule->lefiViaRule::name());
  if (viaRule->lefiViaRule::hasGenerate())
     fprintf(fout, " GENERATE");
  if (viaRule->lefiViaRule::hasDefault())
     fprintf(fout, " DEFAULT");
  fprintf(fout, "\n");

  numLayers = viaRule->lefiViaRule::numLayers();
  // if numLayers == 2, it is VIARULE without GENERATE and has via name
  // if numLayers == 3, it is VIARULE with GENERATE, and the 3rd layer is cut
  for (i = 0; i < numLayers; i++) {
     vLayer = viaRule->lefiViaRule::layer(i);
     lefViaRuleLayer(vLayer);
  }

  if (numLayers == 2 && !(viaRule->lefiViaRule::hasGenerate())) {
     // should have vianames
     numVias = viaRule->lefiViaRule::numVias();
     if (numVias == 0)
        fprintf(fout, "Should have via names in VIARULE.\n");
     else {
        for (i = 0; i < numVias; i++)
           fprintf(fout, IN0 "VIA %s ;\n", viaRule->lefiViaRule::viaName(i));
     }
  }
  if (viaRule->lefiViaRule::numProps() > 0) {
     fprintf(fout, IN0 "PROPERTY ");
     for (i = 0; i < viaRule->lefiViaRule::numProps(); i++) {
        fprintf(fout, "%s ", viaRule->lefiViaRule::propName(i));
        if (viaRule->lefiViaRule::propValue(i))
           fprintf(fout, "%s ", viaRule->lefiViaRule::propValue(i));
        switch (viaRule->lefiViaRule::propType(i)) {
           case 'R': fprintf(fout, "REAL ");
                     break;
           case 'I': fprintf(fout, "INTEGER ");
                     break;
           case 'S': fprintf(fout, "STRING ");
                     break;
           case 'Q': fprintf(fout, "QUOTESTRING ");
                     break;
           case 'N': fprintf(fout, "NUMBER ");
                     break;
        }
     }
     fprintf(fout, ";\n");
  }
  fprintf(fout, "END %s\n", viaRule->lefiViaRule::name());
  return 0;
}

int extensionCB(lefrCallbackType_e c, const char* extsn, lefiUserData ud) {
  checkType(c);
  // lefrSetCaseSensitivity(0);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "BEGINEXT %s ;\n", extsn);
  // lefrSetCaseSensitivity(1);
  return 0;
}

int doneCB(lefrCallbackType_e c, void* ptr, lefiUserData ud) {
  checkType(c);
  // if ((long)ud != userData) dataError();
  fprintf(fout, "END LIBRARY\n");
  return 0;
}

void errorCB(const char* msg) {
  printf ("%s : %s\n", lefrGetUserData(), msg);
}

void warningCB(const char* msg) {
  printf ("%s : %s\n", lefrGetUserData(), msg);
}

void* mallocCB(int size) {
  return malloc(size);
}

void* reallocCB(void* name, int size) {
  return realloc(name, size);
}

void freeCB(void* name) {
  free(name);
  return;
}

void lineNumberCB(int lineNo) {
  //fprintf(fout, "Parsed %d number of lines!!\n", lineNo);
  return;
}

void printWarning(const char *str)
{
    fprintf(stderr, "%s\n", str);
}



int
main(int argc, char** argv) {
  char* inFile[100];
  char* outFile;
  FILE* f;
  int res;
  int noCalls = 0;
//  long start_mem;
  int num;
  int status;
  int retStr = 0;
  int numInFile = 0;
  int fileCt = 0;
  int relax = 0;
  const char* version = "N/A";
  int setVer = 0;
  char* userData;
  int msgCb = 0;
  int test1 = 0;
  int test2 = 0;
  int ccr749853 = 0;
  int verbose = 0;

// start_mem = (long)sbrk(0);

  userData = strdup ("(lefrw-5100)");
  strcpy(defaultName,"lef.in");
  strcpy(defaultOut,"list");
  inFile[0] = defaultName;
  outFile = defaultOut;
  fout = stdout;
//  userData = 0x01020304;

  argc--;
  argv++;
  while (argc--) {

    if (strcmp(*argv, "-d") == 0) {
      argv++;
      argc--;
      sscanf(*argv, "%d", &num);
      lefiSetDebug(num, 1);
    }else if (strcmp(*argv, "-nc") == 0) {
      noCalls = 1;

    } else if (strcmp(*argv, "-p") == 0) {
      printing = 1;

    } else if (strcmp(*argv, "-m") == 0) { // use the user error/warning CB
      msgCb = 1;

    } else if (strcmp(*argv, "-o") == 0) {
      argv++;
      argc--;
      outFile = *argv;
      if ((fout = fopen(outFile, "w")) == 0) {
	fprintf(stderr, "ERROR: could not open output file\n");
	return 2;
      }

    } else if (strcmp(*argv, "-verStr") == 0) {
        /* New to set the version callback routine to return a string    */
        /* instead of double.                                            */
        retStr = 1;

    } else if (strcmp(*argv, "-relax") == 0) {
        relax = 1;

    } else if (strcmp(*argv, "-65nm") == 0) {
        parse65nm = 1;

    } else if (strcmp(*argv, "-lef58") == 0) {
        parseLef58Type = 1;

    } else if (strcmp(*argv, "-ver") == 0) {
      argv++;
      argc--;
      setVer = 1;
      version = *argv;
    } else if (strcmp(*argv, "-test1") == 0) {
      test1 = 1;
    } else if (strcmp(*argv, "-test2") == 0) {
      test2 = 1;
    } else if (strcmp(*argv, "-sessionless") == 0) {
      isSessionles = 1;
    } else if (strcmp(*argv, "-ccr749853") == 0) {
      ccr749853 = 1;
    } else if (argv[0][0] != '-') {
      if (numInFile >= 100) {
        fprintf(stderr, "ERROR: too many input files, max = 3.\n");
        return 2;
      }
      inFile[numInFile++] = *argv;

    } else {
      fprintf(stderr, "ERROR: Illegal command line option: '%s'\n", *argv);
      return 2;
    }

    argv++;
  }

  // sets the parser to be case sensitive...
  // default was supposed to be the case but false...
  // lefrSetCaseSensitivity(true);
  if (isSessionles) {
	lefrSetOpenLogFileAppend();
  }

  lefrInitSession(isSessionles ? 0 : 1);

  if (noCalls == 0) {
     lefrSetWarningLogFunction(printWarning);
     lefrSetAntennaInputCbk(antennaCB);
     lefrSetAntennaInoutCbk(antennaCB);
     lefrSetAntennaOutputCbk(antennaCB);
     lefrSetArrayBeginCbk(arrayBeginCB);
     lefrSetArrayCbk(arrayCB);
     lefrSetArrayEndCbk(arrayEndCB);
     lefrSetBusBitCharsCbk(busBitCharsCB);
     lefrSetCaseSensitiveCbk(caseSensCB);
     lefrSetFixedMaskCbk(fixedMaskCB);
     lefrSetClearanceMeasureCbk(clearanceCB);
     lefrSetDensityCbk(densityCB);
     lefrSetDividerCharCbk(dividerCB);
     lefrSetNoWireExtensionCbk(noWireExtCB);
     lefrSetNoiseMarginCbk(noiseMarCB);
     lefrSetEdgeRateThreshold1Cbk(edge1CB);
     lefrSetEdgeRateThreshold2Cbk(edge2CB);
     lefrSetEdgeRateScaleFactorCbk(edgeScaleCB);
     lefrSetExtensionCbk(extensionCB);
     lefrSetNoiseTableCbk(noiseTableCB);
     lefrSetCorrectionTableCbk(correctionCB);
     lefrSetDielectricCbk(dielectricCB);
     lefrSetIRDropBeginCbk(irdropBeginCB);
     lefrSetIRDropCbk(irdropCB);
     lefrSetIRDropEndCbk(irdropEndCB);
     lefrSetLayerCbk(layerCB);
     lefrSetLibraryEndCbk(doneCB);
     lefrSetMacroBeginCbk(macroBeginCB);
     lefrSetMacroCbk(macroCB);
     lefrSetMacroClassTypeCbk(macroClassTypeCB);
     lefrSetMacroOriginCbk(macroOriginCB);
     lefrSetMacroSizeCbk(macroSizeCB);
     lefrSetMacroFixedMaskCbk(macroFixedMaskCB);
     lefrSetMacroEndCbk(macroEndCB);
     lefrSetManufacturingCbk(manufacturingCB);
     lefrSetMaxStackViaCbk(maxStackViaCB);
     lefrSetMinFeatureCbk(minFeatureCB);
     lefrSetNonDefaultCbk(nonDefaultCB);
     lefrSetObstructionCbk(obstructionCB);
     lefrSetPinCbk(pinCB);
     lefrSetPropBeginCbk(propDefBeginCB);
     lefrSetPropCbk(propDefCB);
     lefrSetPropEndCbk(propDefEndCB);
     lefrSetSiteCbk(siteCB);
     lefrSetSpacingBeginCbk(spacingBeginCB);
     lefrSetSpacingCbk(spacingCB);
     lefrSetSpacingEndCbk(spacingEndCB);
     lefrSetTimingCbk(timingCB);
     lefrSetUnitsCbk(unitsCB);
     lefrSetUseMinSpacingCbk(useMinSpacingCB);
     lefrSetUserData((void*)3);
     if (!retStr)
       lefrSetVersionCbk(versionCB);
     else
       lefrSetVersionStrCbk(versionStrCB);
     lefrSetViaCbk(viaCB);
     lefrSetViaRuleCbk(viaRuleCB);
     lefrSetInputAntennaCbk(antennaCB);
     lefrSetOutputAntennaCbk(antennaCB);
     lefrSetInoutAntennaCbk(antennaCB);

     if (msgCb) {
       lefrSetLogFunction(errorCB);
       lefrSetWarningLogFunction(warningCB);
     }

     lefrSetMallocFunction(mallocCB);
     lefrSetReallocFunction(reallocCB);
     lefrSetFreeFunction(freeCB);

     lefrSetLineNumberFunction(lineNumberCB);
     lefrSetDeltaNumberLines(50);

     lefrSetRegisterUnusedCallbacks();

     if (relax)
       lefrSetRelaxMode();

     if (setVer)
       (void)lefrSetVersionValue(version);

     lefrSetAntennaInoutWarnings(30);
     lefrSetAntennaInputWarnings(30);
     lefrSetAntennaOutputWarnings(30);
     lefrSetArrayWarnings(30);
     lefrSetCaseSensitiveWarnings(30);
     lefrSetCorrectionTableWarnings(30);
     lefrSetDielectricWarnings(30);
     lefrSetEdgeRateThreshold1Warnings(30);
     lefrSetEdgeRateThreshold2Warnings(30);
     lefrSetEdgeRateScaleFactorWarnings(30);
     lefrSetInoutAntennaWarnings(30);
     lefrSetInputAntennaWarnings(30);
     lefrSetIRDropWarnings(30);
     lefrSetLayerWarnings(30);
     lefrSetMacroWarnings(30);
     lefrSetMaxStackViaWarnings(30);
     lefrSetMinFeatureWarnings(30);
     lefrSetNoiseMarginWarnings(30);
     lefrSetNoiseTableWarnings(30);
     lefrSetNonDefaultWarnings(30);
     lefrSetNoWireExtensionWarnings(30);
     lefrSetOutputAntennaWarnings(30);
     lefrSetPinWarnings(30);
     lefrSetSiteWarnings(30);
     lefrSetSpacingWarnings(30);
     lefrSetTimingWarnings(30);
     lefrSetUnitsWarnings(30);
     lefrSetUseMinSpacingWarnings(30);
     lefrSetViaRuleWarnings(30);
     lefrSetViaWarnings(30);
  }

  (void) lefrSetShiftCase();  // will shift name to uppercase if caseinsensitive
                              // is set to off or not set
  if (!isSessionles) {
	lefrSetOpenLogFileAppend();
  }

  if (ccr749853) {
    lefrSetTotalMsgLimit (5);
    lefrSetLimitPerMsg (1618, 2);
  }

  if (test1) {  // for special tests
    for (fileCt = 0; fileCt < numInFile; fileCt++) {
      lefrReset();

      if ((f = fopen(inFile[fileCt],"r")) == 0) {
        fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
        return(2);
      }

      (void)lefrEnableReadEncrypted();

      status = lefwInit(fout); // initialize the lef writer,
                               // need to be called 1st
      if (status != LEFW_OK)
         return 1;

      res = lefrRead(f, inFile[fileCt], (void*)userData);

      if (res)
         fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

      (void)lefrPrintUnusedCallbacks(fout);
      (void)lefrReleaseNResetMemory();
      //(void)lefrUnsetCallbacks();
      (void)lefrUnsetLayerCbk();
      (void)lefrUnsetNonDefaultCbk();
      (void)lefrUnsetViaCbk();

    }
  }
  else if (test2) {  // for special tests
    // this test is design to test the 3 APIs, lefrDisableParserMsgs,
    // lefrEnableParserMsgs & lefrEnableAllMsgs
    // It uses the file ccr566209.lef.  This file will parser 3 times
    // 1st it will have lefrDisableParserMsgs set to both 2007 & 2008
    // 2nd will enable 2007 by calling lefrEnableParserMsgs
    // 3rd enable all msgs by call lefrEnableAllMsgs

    int nMsgs = 3;
    int dMsgs[3];
    if (numInFile != 1) {
        fprintf(stderr,"Test 2 mode needs only 1 file\n");
        return 2;
    }

    for (int idx=0; idx<5; idx++) {
      if (idx == 0) {  // msgs 2005 & 2011
         fprintf(stderr,"\nPass 0: Disabling 2007, 2008, 2009\n");
         dMsgs[0] = 2007;
         dMsgs[1] = 2008;
         dMsgs[2] = 2009;
         lefrDisableParserMsgs (3, (int*)dMsgs);
      } else if (idx == 1) { // msgs 2007 & 2005, 2011 did not print because
         fprintf(stderr,"\nPass 1: Enable 2007\n");
         dMsgs[0] = 2007;       // lefrUnsetLayerCbk() was called
         lefrEnableParserMsgs (1, (int*)dMsgs);
      } else if (idx == 2) { // nothing were printed
         fprintf(stderr,"\nPass 2: Disable all\n");
         lefrDisableAllMsgs();
      } else if (idx == 3) { // nothing were printed, lefrDisableParserMsgs
         fprintf(stderr,"\nPass 3: Enable All\n");
         lefrEnableAllMsgs();
      } else if (idx == 4) { // msgs 2005 was printed
         fprintf(stderr,"\nPass 4: Set limit on 2007 up 2\n");
         lefrSetLimitPerMsg (2007, 2);
      }

      if ((f = fopen(inFile[fileCt],"r")) == 0) {
        fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
        return(2);
      }

      (void)lefrEnableReadEncrypted();

      status = lefwInit(fout); // initialize the lef writer,
                               // need to be called 1st
      if (status != LEFW_OK)
         return 1;

      res = lefrRead(f, inFile[fileCt], (void*)userData);

      if (res)
         fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

      (void)lefrPrintUnusedCallbacks(fout);
      (void)lefrReleaseNResetMemory();
      //(void)lefrUnsetCallbacks();
      (void)lefrUnsetLayerCbk();
      (void)lefrUnsetNonDefaultCbk();
      (void)lefrUnsetViaCbk();

    }
  } else {
    for (fileCt = 0; fileCt < numInFile; fileCt++) {
      lefrReset();

      if ((f = fopen(inFile[fileCt],"r")) == 0) {
        fprintf(stderr,"Couldn't open input file '%s'\n", inFile[fileCt]);
        return(2);
      }

      (void)lefrEnableReadEncrypted();

      status = lefwInit(fout); // initialize the lef writer,
                               // need to be called 1st
      if (status != LEFW_OK)
         return 1;

      res = lefrRead(f, inFile[fileCt], (void*)userData);

      if (res)
         fprintf(stderr, "Reader returns bad status.\n", inFile[fileCt]);

      (void)lefrPrintUnusedCallbacks(fout);
      (void)lefrReleaseNResetMemory();

    }
    (void)lefrUnsetCallbacks();
  }
  // Unset all the callbacks
  void lefrUnsetAntennaInputCbk();
  void lefrUnsetAntennaInoutCbk();
  void lefrUnsetAntennaOutputCbk();
  void lefrUnsetArrayBeginCbk();
  void lefrUnsetArrayCbk();
  void lefrUnsetArrayEndCbk();
  void lefrUnsetBusBitCharsCbk();
  void lefrUnsetCaseSensitiveCbk();
  void lefrUnsetFixedMaskCbk();
  void lefrUnsetClearanceMeasureCbk();
  void lefrUnsetCorrectionTableCbk();
  void lefrUnsetDensityCbk();
  void lefrUnsetDielectricCbk();
  void lefrUnsetDividerCharCbk();
  void lefrUnsetEdgeRateScaleFactorCbk();
  void lefrUnsetEdgeRateThreshold1Cbk();
  void lefrUnsetEdgeRateThreshold2Cbk();
  void lefrUnsetExtensionCbk();
  void lefrUnsetInoutAntennaCbk();
  void lefrUnsetInputAntennaCbk();
  void lefrUnsetIRDropBeginCbk();
  void lefrUnsetIRDropCbk();
  void lefrUnsetIRDropEndCbk();
  void lefrUnsetLayerCbk();
  void lefrUnsetLibraryEndCbk();
  void lefrUnsetMacroBeginCbk();
  void lefrUnsetMacroCbk();
  void lefrUnsetMacroClassTypeCbk();
  void lefrUnsetMacroEndCbk();
  void lefrUnsetMacroOriginCbk();
  void lefrUnsetMacroSizeCbk();
  void lefrUnsetManufacturingCbk();
  void lefrUnsetMaxStackViaCbk();
  void lefrUnsetMinFeatureCbk();
  void lefrUnsetNoiseMarginCbk();
  void lefrUnsetNoiseTableCbk();
  void lefrUnsetNonDefaultCbk();
  void lefrUnsetNoWireExtensionCbk();
  void lefrUnsetObstructionCbk();
  void lefrUnsetOutputAntennaCbk();
  void lefrUnsetPinCbk();
  void lefrUnsetPropBeginCbk();
  void lefrUnsetPropCbk();
  void lefrUnsetPropEndCbk();
  void lefrUnsetSiteCbk();
  void lefrUnsetSpacingBeginCbk();
  void lefrUnsetSpacingCbk();
  void lefrUnsetSpacingEndCbk();
  void lefrUnsetTimingCbk();
  void lefrUnsetUseMinSpacingCbk();
  void lefrUnsetUnitsCbk();
  void lefrUnsetVersionCbk();
  void lefrUnsetVersionStrCbk();
  void lefrUnsetViaCbk();
  void lefrUnsetViaRuleCbk();

  fclose(fout);

  // Release allocated singleton data.
  lefrClear();

  return 0;
}

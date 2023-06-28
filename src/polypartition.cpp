/*************************************************************************/
/* Copyright (c) 2011-2021 Ivan Fratric and contributors.                */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "polypartition.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

void TPPLPoly::Clear() {
  hole = false;
  points.clear();
}

void TPPLPoly::Init(const tppl_idx numpoints) {
  Clear();
  points.resize(std::vector<TPPLPoint>::size_type(numpoints));
}

void TPPLPoly::Triangle(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3) {
  Init(3);
  points[0] = p1;
  points[1] = p2;
  points[2] = p3;
}

TPPLOrientation TPPLPoly::GetOrientation() const {
  tppl_float area = 0;
  auto const numpoints = GetNumPoints();
  for (tppl_idx i1 = 0; i1 < numpoints; i1++) {
    tppl_idx i2 = i1 + 1;
    if (i2 == numpoints) {
      i2 = 0;
    }
    area += points[i1].x * points[i2].y - points[i1].y * points[i2].x;
  }
  if (area > 0) {
    return TPPL_ORIENTATION_CCW;
  }
  if (area < 0) {
    return TPPL_ORIENTATION_CW;
  }
  return TPPL_ORIENTATION_NONE;
}

void TPPLPoly::SetOrientation(const TPPLOrientation orientation) {
  TPPLOrientation polyorientation = GetOrientation();
  if (polyorientation != TPPL_ORIENTATION_NONE && polyorientation != orientation) {
    Invert();
  }
}

void TPPLPoly::Invert() {
  std::reverse(points.begin(), points.end());
}

TPPLPoint TPPLPartition::Normalize(const TPPLPoint &p) {
  TPPLPoint r;
  const tppl_float n = sqrt(p.x * p.x + p.y * p.y);
  if (n != 0) {
    r = p / n;
  } else {
    r.x = 0;
    r.y = 0;
  }
  return r;
}

tppl_float TPPLPartition::Distance(const TPPLPoint &p1, const TPPLPoint &p2) {
  const tppl_float dx = p2.x - p1.x;
  const tppl_float dy = p2.y - p1.y;
  return (sqrt(dx * dx + dy * dy));
}

// Checks if two lines intersect.
int TPPLPartition::Intersects(const TPPLPoint &p11, const TPPLPoint &p12, const TPPLPoint &p21, const TPPLPoint &p22) {
  if (p11 == p21 || p11 == p22 || p12 == p21 || p12 == p22) {
    return 0;
  }

  TPPLPoint v1ort, v2ort;

  v1ort.x = p12.y - p11.y;
  v1ort.y = p11.x - p12.x;

  v2ort.x = p22.y - p21.y;
  v2ort.y = p21.x - p22.x;

  TPPLPoint v = p21 - p11;
  const tppl_float dot21 = v.x * v1ort.x + v.y * v1ort.y;
  v = p22 - p11;
  const tppl_float dot22 = v.x * v1ort.x + v.y * v1ort.y;

  v = p11 - p21;
  const tppl_float dot11 = v.x * v2ort.x + v.y * v2ort.y;
  v = p12 - p21;
  const tppl_float dot12 = v.x * v2ort.x + v.y * v2ort.y;

  if (dot11 * dot12 > 0 || dot21 * dot22 > 0) {
    return 0;
  }
  return 1;
}

// Removes holes from inpolys by merging them with non-holes.
int TPPLPartition::RemoveHoles(TPPLPolyList *inpolys, TPPLPolyList *outpolys) {
  TPPLPolyList polys;
  TPPLPolyList::iterator holeiter, polyiter, iter, iter2;
  tppl_idx i, i2, holepointindex, polypointindex;
  TPPLPoint holepoint, polypoint, bestpolypoint;
  TPPLPoint linep1, linep2;
  TPPLPoint v1, v2;
  TPPLPoly newpoly;
  bool pointvisible;
  bool pointfound;

  // Check for the trivial case of no holes.
  bool hasholes = false;
  for (iter = inpolys->begin(); iter != inpolys->end(); ++iter) {
    if (iter->IsHole()) {
      hasholes = true;
      break;
    }
  }
  if (!hasholes) {
    for (iter = inpolys->begin(); iter != inpolys->end(); ++iter) {
      outpolys->push_back(*iter);
    }
    return 1;
  }

  polys = *inpolys;

  while (true) {
    // Find the hole point with the largest x.
    hasholes = false;
    for (iter = polys.begin(); iter != polys.end(); ++iter) {
      if (!iter->IsHole()) {
        continue;
      }

      if (!hasholes) {
        hasholes = true;
        holeiter = iter;
        holepointindex = 0;
      }

      for (i = 0; i < iter->GetNumPoints(); i++) {
        if (iter->GetPoint(i).x > holeiter->GetPoint(holepointindex).x) {
          holeiter = iter;
          holepointindex = i;
        }
      }
    }
    if (!hasholes) {
      break;
    }
    holepoint = holeiter->GetPoint(holepointindex);

    pointfound = false;
    for (iter = polys.begin(); iter != polys.end(); ++iter) {
      if (iter->IsHole()) {
        continue;
      }
      for (i = 0; i < iter->GetNumPoints(); i++) {
        if (iter->GetPoint(i).x <= holepoint.x) {
          continue;
        }
        if (!InCone(iter->GetPoint((i + iter->GetNumPoints() - 1) % (iter->GetNumPoints())),
                    iter->GetPoint(i),
                    iter->GetPoint((i + 1) % (iter->GetNumPoints())),
                    holepoint)) {
          continue;
        }
        polypoint = iter->GetPoint(i);
        if (pointfound) {
          v1 = Normalize(polypoint - holepoint);
          v2 = Normalize(bestpolypoint - holepoint);
          if (v2.x > v1.x) {
            continue;
          }
        }
        pointvisible = true;
        for (iter2 = polys.begin(); iter2 != polys.end(); ++iter2) {
          if (iter2->IsHole()) {
            continue;
          }
          for (i2 = 0; i2 < iter2->GetNumPoints(); i2++) {
            linep1 = iter2->GetPoint(i2);
            linep2 = iter2->GetPoint((i2 + 1) % (iter2->GetNumPoints()));
            if (Intersects(holepoint, polypoint, linep1, linep2)) {
              pointvisible = false;
              break;
            }
          }
          if (!pointvisible) {
            break;
          }
        }
        if (pointvisible) {
          pointfound = true;
          bestpolypoint = polypoint;
          polyiter = iter;
          polypointindex = i;
        }
      }
    }

    if (!pointfound) {
      return 0;
    }

    newpoly.Init(holeiter->GetNumPoints() + polyiter->GetNumPoints() + 2);
    i2 = 0;
    for (i = 0; i <= polypointindex; i++) {
      newpoly[i2] = polyiter->GetPoint(i);
      i2++;
    }
    for (i = 0; i <= holeiter->GetNumPoints(); i++) {
      newpoly[i2] = holeiter->GetPoint((i + holepointindex) % holeiter->GetNumPoints());
      i2++;
    }
    for (i = polypointindex; i < polyiter->GetNumPoints(); i++) {
      newpoly[i2] = polyiter->GetPoint(i);
      i2++;
    }

    polys.erase(holeiter);
    polys.erase(polyiter);
    polys.push_back(newpoly);
  }

  for (iter = polys.begin(); iter != polys.end(); ++iter) {
    outpolys->push_back(*iter);
  }

  return 1;
}

bool TPPLPartition::IsConvex(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3) {
  const tppl_float tmp = (p3.y - p1.y) * (p2.x - p1.x) - (p3.x - p1.x) * (p2.y - p1.y);
  return tmp > 0;
}

bool TPPLPartition::IsReflex(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3) {
  const tppl_float tmp = (p3.y - p1.y) * (p2.x - p1.x) - (p3.x - p1.x) * (p2.y - p1.y);
  return tmp < 0;
}

bool TPPLPartition::IsInside(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3, const TPPLPoint &p) {
  if (IsConvex(p1, p, p2)) {
    return false;
  }
  if (IsConvex(p2, p, p3)) {
    return false;
  }
  if (IsConvex(p3, p, p1)) {
    return false;
  }
  return true;
}

bool TPPLPartition::InCone(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3, const TPPLPoint &p) {
  const bool convex = IsConvex(p1, p2, p3);

  if (convex) {
    if (!IsConvex(p1, p2, p)) {
      return false;
    }
    if (!IsConvex(p2, p3, p)) {
      return false;
    }
    return true;
  } else {
    if (IsConvex(p1, p2, p)) {
      return true;
    }
    if (IsConvex(p2, p3, p)) {
      return true;
    }
    return false;
  }
}

bool TPPLPartition::InCone(const PartitionVertex *v, const TPPLPoint &p) {
  return InCone(v->previous->p, v->p, v->next->p, p);
}

void TPPLPartition::UpdateVertexReflexity(PartitionVertex *v) {
  v->isConvex = !IsReflex(v->previous->p, v->p, v->next->p);
}

void TPPLPartition::UpdateVertex(PartitionVertex *v, const PartitionVertex *vertices, const tppl_idx numvertices) {
  const PartitionVertex *v1 = v->previous;
  const PartitionVertex *v3 = v->next;

  v->isConvex = IsConvex(v1->p, v->p, v3->p);

  const TPPLPoint vec1 = Normalize(v1->p - v->p);
  const TPPLPoint vec3 = Normalize(v3->p - v->p);
  v->angle = vec1.x * vec3.x + vec1.y * vec3.y;

  if (v->isConvex) {
    v->isEar = true;
    for (tppl_idx i = 0; i < numvertices; i++) {
      if (vertices[i].p == v->p || vertices[i].p == v1->p || vertices[i].p == v3->p) {
        continue;
      }
      if (IsInside(v1->p, v->p, v3->p, vertices[i].p)) {
        v->isEar = false;
        break;
      }
    }
  } else {
    v->isEar = false;
  }
}

// Triangulation by ear removal.
int TPPLPartition::Triangulate_EC(TPPLPoly *poly, TPPLPolyList *triangles) {
  if (!poly->Valid()) {
    return 0;
  }

  PartitionVertex *ear = nullptr;
  TPPLPoly triangle;
  tppl_idx i;

  if (poly->GetNumPoints() < 3) {
    return 0;
  }
  if (poly->GetNumPoints() == 3) {
    triangles->push_back(*poly);
    return 1;
  }

  tppl_idx numvertices = poly->GetNumPoints();

  auto vertices = new PartitionVertex[numvertices];
  for (i = 0; i < numvertices; i++) {
    vertices[i].isActive = true;
    vertices[i].p = poly->GetPoint(i);
    if (i == (numvertices - 1)) {
      vertices[i].next = &(vertices[0]);
    } else {
      vertices[i].next = &(vertices[i + 1]);
    }
    if (i == 0) {
      vertices[i].previous = &(vertices[numvertices - 1]);
    } else {
      vertices[i].previous = &(vertices[i - 1]);
    }
  }
  for (i = 0; i < numvertices; i++) {
    UpdateVertex(&vertices[i], vertices, numvertices);
  }

  for (i = 0; i < numvertices - 3; i++) {
    bool earfound = false;
    // Find the most extruded ear.
    for (tppl_idx j = 0; j < numvertices; j++) {
      if (!vertices[j].isActive) {
        continue;
      }
      if (!vertices[j].isEar) {
        continue;
      }
      if (!earfound) {
        earfound = true;
        ear = &(vertices[j]);
      } else {
        if (vertices[j].angle > ear->angle) {
          ear = &(vertices[j]);
        }
      }
    }
    if (!earfound) {
      delete[] vertices;
      return 0;
    }

    triangle.Triangle(ear->previous->p, ear->p, ear->next->p);
    triangles->push_back(triangle);

    ear->isActive = false;
    ear->previous->next = ear->next;
    ear->next->previous = ear->previous;

    if (i == numvertices - 4) {
      break;
    }

    UpdateVertex(ear->previous, vertices, numvertices);
    UpdateVertex(ear->next, vertices, numvertices);
  }
  for (i = 0; i < numvertices; i++) {
    if (vertices[i].isActive) {
      triangle.Triangle(vertices[i].previous->p, vertices[i].p, vertices[i].next->p);
      triangles->push_back(triangle);
      break;
    }
  }

  delete[] vertices;

  return 1;
}

int TPPLPartition::Triangulate_EC(TPPLPolyList *inpolys, TPPLPolyList *triangles) {
  TPPLPolyList outpolys;

  if (!RemoveHoles(inpolys, &outpolys)) {
    return 0;
  }
  for (auto iter = outpolys.begin(); iter != outpolys.end(); ++iter) {
    if (!Triangulate_EC(&(*iter), triangles)) {
      return 0;
    }
  }
  return 1;
}

int TPPLPartition::ConvexPartition_HM(TPPLPoly *poly, TPPLPolyList *parts) {
  if (!poly->Valid()) {
    return 0;
  }

  TPPLPolyList triangles;
  TPPLPolyList::iterator iter1, iter2;
  TPPLPoly *poly1 = nullptr, *poly2 = nullptr;
  TPPLPoly newpoly;
  TPPLPoint d1, d2, p1, p2, p3;
  tppl_idx i11, i12, i21, i22, i13, i23, j, k;
  bool isdiagonal;

  // Check if the poly is already convex.
  tppl_idx numreflex = 0;
  for (i11 = 0; i11 < poly->GetNumPoints(); i11++) {
    if (i11 == 0) {
      i12 = poly->GetNumPoints() - 1;
    } else {
      i12 = i11 - 1;
    }
    if (i11 == (poly->GetNumPoints() - 1)) {
      i13 = 0;
    } else {
      i13 = i11 + 1;
    }
    if (IsReflex(poly->GetPoint(i12), poly->GetPoint(i11), poly->GetPoint(i13))) {
      numreflex = 1;
      break;
    }
  }
  if (numreflex == 0) {
    parts->push_back(*poly);
    return 1;
  }

  if (!Triangulate_EC(poly, &triangles)) {
    return 0;
  }

  for (iter1 = triangles.begin(); iter1 != triangles.end(); ++iter1) {
    poly1 = &(*iter1);
    for (i11 = 0; i11 < poly1->GetNumPoints(); i11++) {
      d1 = poly1->GetPoint(i11);
      i12 = (i11 + 1) % (poly1->GetNumPoints());
      d2 = poly1->GetPoint(i12);

      isdiagonal = false;
      for (iter2 = iter1; iter2 != triangles.end(); ++iter2) {
        if (iter1 == iter2) {
          continue;
        }
        poly2 = &(*iter2);

        for (i21 = 0; i21 < poly2->GetNumPoints(); i21++) {
          if (d2 != poly2->GetPoint(i21)) {
            continue;
          }
          i22 = (i21 + 1) % (poly2->GetNumPoints());
          if (d1 != poly2->GetPoint(i22)) {
            continue;
          }
          isdiagonal = true;
          break;
        }
        if (isdiagonal) {
          break;
        }
      }

      if (!isdiagonal) {
        continue;
      }

      p2 = poly1->GetPoint(i11);
      if (i11 == 0) {
        i13 = poly1->GetNumPoints() - 1;
      } else {
        i13 = i11 - 1;
      }
      p1 = poly1->GetPoint(i13);
      if (i22 == (poly2->GetNumPoints() - 1)) {
        i23 = 0;
      } else {
        i23 = i22 + 1;
      }
      p3 = poly2->GetPoint(i23);

      if (!IsConvex(p1, p2, p3)) {
        continue;
      }

      p2 = poly1->GetPoint(i12);
      if (i12 == (poly1->GetNumPoints() - 1)) {
        i13 = 0;
      } else {
        i13 = i12 + 1;
      }
      p3 = poly1->GetPoint(i13);
      if (i21 == 0) {
        i23 = poly2->GetNumPoints() - 1;
      } else {
        i23 = i21 - 1;
      }
      p1 = poly2->GetPoint(i23);

      if (!IsConvex(p1, p2, p3)) {
        continue;
      }

      newpoly.Init(poly1->GetNumPoints() + poly2->GetNumPoints() - 2);
      k = 0;
      for (j = i12; j != i11; j = (j + 1) % (poly1->GetNumPoints())) {
        newpoly[k] = poly1->GetPoint(j);
        k++;
      }
      for (j = i22; j != i21; j = (j + 1) % (poly2->GetNumPoints())) {
        newpoly[k] = poly2->GetPoint(j);
        k++;
      }

      triangles.erase(iter2);
      *iter1 = newpoly;
      poly1 = &(*iter1);
      i11 = -1;
    }
  }

  for (iter1 = triangles.begin(); iter1 != triangles.end(); ++iter1) {
    parts->push_back(*iter1);
  }

  return 1;
}

int TPPLPartition::ConvexPartition_HM(TPPLPolyList *inpolys, TPPLPolyList *parts) {
  TPPLPolyList outpolys;

  if (!RemoveHoles(inpolys, &outpolys)) {
    return 0;
  }
  for (auto iter = outpolys.begin(); iter != outpolys.end(); ++iter) {
    if (!ConvexPartition_HM(&(*iter), parts)) {
      return 0;
    }
  }
  return 1;
}

// Minimum-weight polygon triangulation by dynamic programming.
// Time complexity: O(n^3)
// Space complexity: O(n^2)
int TPPLPartition::Triangulate_OPT(TPPLPoly *poly, TPPLPolyList *triangles) {
  if (!poly->Valid()) {
    return 0;
  }

  tppl_idx i, j, k, gap;
  TPPLPoint p1, p2, p3, p4;
  tppl_idx bestvertex;
  tppl_float weight, minweight, d1, d2;
  DiagonalList diagonals;
  TPPLPoly triangle;
  int ret = 1;

  tppl_idx n = poly->GetNumPoints();
  auto dpstates = new DPState *[n];
  for (i = 1; i < n; i++) {
    dpstates[i] = new DPState[i];
  }

  // Initialize states and visibility.
  for (i = 0; i < (n - 1); i++) {
    p1 = poly->GetPoint(i);
    for (j = i + 1; j < n; j++) {
      dpstates[j][i].visible = true;
      dpstates[j][i].weight = 0;
      dpstates[j][i].bestvertex = -1;
      if (j != (i + 1)) {
        p2 = poly->GetPoint(j);

        // Visibility check.
        if (i == 0) {
          p3 = poly->GetPoint(n - 1);
        } else {
          p3 = poly->GetPoint(i - 1);
        }
        if (i == (n - 1)) {
          p4 = poly->GetPoint(0);
        } else {
          p4 = poly->GetPoint(i + 1);
        }
        if (!InCone(p3, p1, p4, p2)) {
          dpstates[j][i].visible = false;
          continue;
        }

        if (j == 0) {
          p3 = poly->GetPoint(n - 1);
        } else {
          p3 = poly->GetPoint(j - 1);
        }
        if (j == (n - 1)) {
          p4 = poly->GetPoint(0);
        } else {
          p4 = poly->GetPoint(j + 1);
        }
        if (!InCone(p3, p2, p4, p1)) {
          dpstates[j][i].visible = false;
          continue;
        }

        for (k = 0; k < n; k++) {
          p3 = poly->GetPoint(k);
          if (k == (n - 1)) {
            p4 = poly->GetPoint(0);
          } else {
            p4 = poly->GetPoint(k + 1);
          }
          if (Intersects(p1, p2, p3, p4)) {
            dpstates[j][i].visible = false;
            break;
          }
        }
      }
    }
  }
  dpstates[n - 1][0].visible = true;
  dpstates[n - 1][0].weight = 0;
  dpstates[n - 1][0].bestvertex = -1;

  for (gap = 2; gap < n; gap++) {
    for (i = 0; i < (n - gap); i++) {
      j = i + gap;
      if (!dpstates[j][i].visible) {
        continue;
      }
      bestvertex = -1;
      for (k = (i + 1); k < j; k++) {
        if (!dpstates[k][i].visible) {
          continue;
        }
        if (!dpstates[j][k].visible) {
          continue;
        }

        if (k <= (i + 1)) {
          d1 = 0;
        } else {
          d1 = Distance(poly->GetPoint(i), poly->GetPoint(k));
        }
        if (j <= (k + 1)) {
          d2 = 0;
        } else {
          d2 = Distance(poly->GetPoint(k), poly->GetPoint(j));
        }

        weight = dpstates[k][i].weight + dpstates[j][k].weight + d1 + d2;

        if ((bestvertex == -1) || (weight < minweight)) {
          bestvertex = k;
          minweight = weight;
        }
      }
      if (bestvertex == -1) {
        for (i = 1; i < n; i++) {
          delete[] dpstates[i];
        }
        delete[] dpstates;

        return 0;
      }

      dpstates[j][i].bestvertex = bestvertex;
      dpstates[j][i].weight = minweight;
    }
  }

  diagonals.emplace_back(0, n - 1);
  while (!diagonals.empty()) {
    Diagonal diagonal = *(diagonals.begin());
    diagonals.pop_front();
    bestvertex = dpstates[diagonal.index2][diagonal.index1].bestvertex;
    if (bestvertex == -1) {
      ret = 0;
      break;
    }
    triangle.Triangle(poly->GetPoint(diagonal.index1), poly->GetPoint(bestvertex), poly->GetPoint(diagonal.index2));
    triangles->push_back(triangle);
    if (bestvertex > (diagonal.index1 + 1)) {
      diagonals.emplace_back(diagonal.index1, bestvertex);
    }
    if (diagonal.index2 > (bestvertex + 1)) {
      diagonals.emplace_back(bestvertex, diagonal.index2);
    }
  }

  for (i = 1; i < n; i++) {
    delete[] dpstates[i];
  }
  delete[] dpstates;

  return ret;
}

void TPPLPartition::UpdateState(const tppl_idx a, const tppl_idx b, const tppl_idx w, const tppl_idx i, const tppl_idx j, DPState2 **dpstates) {
  const tppl_idx w2 = dpstates[a][b].weight;
  if (w > w2) {
    return;
  }

  const auto pairs = &(dpstates[a][b].pairs);
  if (w < w2) {
    pairs->clear();
    pairs->emplace_front(i, j);
    dpstates[a][b].weight = w;
  } else {
    if ((!pairs->empty()) && (i <= pairs->begin()->index1)) {
      return;
    }
    while ((!pairs->empty()) && (pairs->begin()->index2 >= j)) {
      pairs->pop_front();
    }
    pairs->emplace_front(i, j);
  }
}

void TPPLPartition::TypeA(const tppl_idx i, const tppl_idx j, const tppl_idx k, const PartitionVertex *vertices, DPState2 **dpstates) {
  if (!dpstates[i][j].visible) {
    return;
  }
  tppl_idx top = j;
  tppl_idx w = dpstates[i][j].weight;
  if (k - j > 1) {
    if (!dpstates[j][k].visible) {
      return;
    }
    w += dpstates[j][k].weight + 1;
  }
  if (j - i > 1) {
    const auto pairs = &(dpstates[i][j].pairs);
    auto iter = pairs->end();
    auto lastiter = pairs->end();
    while (iter != pairs->begin()) {
      --iter;
      if (!IsReflex(vertices[iter->index2].p, vertices[j].p, vertices[k].p)) {
        lastiter = iter;
      } else {
        break;
      }
    }
    if (lastiter == pairs->end()) {
      w++;
    } else {
      if (IsReflex(vertices[k].p, vertices[i].p, vertices[lastiter->index1].p)) {
        w++;
      } else {
        top = lastiter->index1;
      }
    }
  }
  UpdateState(i, k, w, top, j, dpstates);
}

void TPPLPartition::TypeB(const tppl_idx i, const tppl_idx j, const tppl_idx k, const PartitionVertex *vertices, DPState2 **dpstates) {
  if (!dpstates[j][k].visible) {
    return;
  }
  tppl_idx top = j;
  tppl_idx w = dpstates[j][k].weight;

  if (j - i > 1) {
    if (!dpstates[i][j].visible) {
      return;
    }
    w += dpstates[i][j].weight + 1;
  }
  if (k - j > 1) {
    DiagonalList *pairs = &(dpstates[j][k].pairs);

    auto iter = pairs->begin();
    if ((!pairs->empty()) && (!IsReflex(vertices[i].p, vertices[j].p, vertices[iter->index1].p))) {
      std::list<Diagonal>::iterator lastiter = iter;
      while (iter != pairs->end()) {
        if (!IsReflex(vertices[i].p, vertices[j].p, vertices[iter->index1].p)) {
          lastiter = iter;
          ++iter;
        } else {
          break;
        }
      }
      if (IsReflex(vertices[lastiter->index2].p, vertices[k].p, vertices[i].p)) {
        w++;
      } else {
        top = lastiter->index2;
      }
    } else {
      w++;
    }
  }
  UpdateState(i, k, w, j, top, dpstates);
}

int TPPLPartition::ConvexPartition_OPT(TPPLPoly *poly, TPPLPolyList *parts) {
  if (!poly->Valid()) {
    return 0;
  }

  TPPLPoint p1, p2, p3, p4;
  PartitionVertex *vertices = nullptr;
  DPState2 **dpstates = nullptr;
  tppl_idx i, j, k, n, gap;
  DiagonalList diagonals, diagonals2;
  DiagonalList *pairs = nullptr, *pairs2 = nullptr;
  DiagonalList::iterator iter, iter2;
  int ret;
  TPPLPoly newpoly;
  std::vector<tppl_idx> indices;
  std::vector<tppl_idx>::iterator iiter;
  bool ijreal, jkreal;

  n = poly->GetNumPoints();
  vertices = new PartitionVertex[n];

  dpstates = new DPState2 *[n];
  for (i = 0; i < n; i++) {
    dpstates[i] = new DPState2[n];
  }

  // Initialize vertex information.
  for (i = 0; i < n; i++) {
    vertices[i].p = poly->GetPoint(i);
    vertices[i].isActive = true;
    if (i == 0) {
      vertices[i].previous = &(vertices[n - 1]);
    } else {
      vertices[i].previous = &(vertices[i - 1]);
    }
    if (i == (poly->GetNumPoints() - 1)) {
      vertices[i].next = &(vertices[0]);
    } else {
      vertices[i].next = &(vertices[i + 1]);
    }
  }
  for (i = 1; i < n; i++) {
    UpdateVertexReflexity(&(vertices[i]));
  }

  // Initialize states and visibility.
  for (i = 0; i < (n - 1); i++) {
    p1 = poly->GetPoint(i);
    for (j = i + 1; j < n; j++) {
      dpstates[i][j].visible = true;
      if (j == i + 1) {
        dpstates[i][j].weight = 0;
      } else {
        dpstates[i][j].weight = 2147483647;
      }
      if (j != (i + 1)) {
        p2 = poly->GetPoint(j);

        // Visibility check.
        if (!InCone(&vertices[i], p2)) {
          dpstates[i][j].visible = false;
          continue;
        }
        if (!InCone(&vertices[j], p1)) {
          dpstates[i][j].visible = false;
          continue;
        }

        for (k = 0; k < n; k++) {
          p3 = poly->GetPoint(k);
          if (k == (n - 1)) {
            p4 = poly->GetPoint(0);
          } else {
            p4 = poly->GetPoint(k + 1);
          }
          if (Intersects(p1, p2, p3, p4)) {
            dpstates[i][j].visible = false;
            break;
          }
        }
      }
    }
  }
  for (i = 0; i < (n - 2); i++) {
    j = i + 2;
    if (dpstates[i][j].visible) {
      dpstates[i][j].weight = 0;
      dpstates[i][j].pairs.emplace_back(i + 1, i + 1);
    }
  }

  dpstates[0][n - 1].visible = true;
  vertices[0].isConvex = false; // By convention.

  for (gap = 3; gap < n; gap++) {
    for (i = 0; i < n - gap; i++) {
      if (vertices[i].isConvex) {
        continue;
      }
      k = i + gap;
      if (dpstates[i][k].visible) {
        if (!vertices[k].isConvex) {
          for (j = i + 1; j < k; j++) {
            TypeA(i, j, k, vertices, dpstates);
          }
        } else {
          for (j = i + 1; j < (k - 1); j++) {
            if (vertices[j].isConvex) {
              continue;
            }
            TypeA(i, j, k, vertices, dpstates);
          }
          TypeA(i, k - 1, k, vertices, dpstates);
        }
      }
    }
    for (k = gap; k < n; k++) {
      if (vertices[k].isConvex) {
        continue;
      }
      i = k - gap;
      if ((vertices[i].isConvex) && (dpstates[i][k].visible)) {
        TypeB(i, i + 1, k, vertices, dpstates);
        for (j = i + 2; j < k; j++) {
          if (vertices[j].isConvex) {
            continue;
          }
          TypeB(i, j, k, vertices, dpstates);
        }
      }
    }
  }

  // Recover solution.
  ret = 1;
  diagonals.emplace_front(0, n - 1);
  while (!diagonals.empty()) {
    Diagonal diagonal = *(diagonals.begin());
    diagonals.pop_front();
    if ((diagonal.index2 - diagonal.index1) <= 1) {
      continue;
    }
    pairs = &(dpstates[diagonal.index1][diagonal.index2].pairs);
    if (pairs->empty()) {
      ret = 0;
      break;
    }
    if (!vertices[diagonal.index1].isConvex) {
      iter = pairs->end();
      --iter;
      j = iter->index2;
      diagonals.emplace_front(j, diagonal.index2);
      if ((j - diagonal.index1) > 1) {
        if (iter->index1 != iter->index2) {
          pairs2 = &(dpstates[diagonal.index1][j].pairs);
          while (true) {
            if (pairs2->empty()) {
              ret = 0;
              break;
            }
            iter2 = pairs2->end();
            --iter2;
            if (iter->index1 != iter2->index1) {
              pairs2->pop_back();
            } else {
              break;
            }
          }
          if (ret == 0) {
            break;
          }
        }
        diagonals.emplace_front(diagonal.index1, j);
      }
    } else {
      iter = pairs->begin();
      j = iter->index1;
      diagonals.emplace_front(diagonal.index1, j);
      if ((diagonal.index2 - j) > 1) {
        if (iter->index1 != iter->index2) {
          pairs2 = &(dpstates[j][diagonal.index2].pairs);
          while (true) {
            if (pairs2->empty()) {
              ret = 0;
              break;
            }
            iter2 = pairs2->begin();
            if (iter->index2 != iter2->index2) {
              pairs2->pop_front();
            } else {
              break;
            }
          }
          if (ret == 0) {
            break;
          }
        }
        diagonals.emplace_front(j, diagonal.index2);
      }
    }
  }

  if (ret == 0) {
    for (i = 0; i < n; i++) {
      delete[] dpstates[i];
    }
    delete[] dpstates;
    delete[] vertices;

    return ret;
  }

  diagonals.emplace_front(0, n - 1);
  while (!diagonals.empty()) {
    Diagonal diagonal = *(diagonals.begin());
    diagonals.pop_front();
    if ((diagonal.index2 - diagonal.index1) <= 1) {
      continue;
    }

    indices.clear();
    diagonals2.clear();
    indices.push_back(diagonal.index1);
    indices.push_back(diagonal.index2);
    diagonals2.push_front(diagonal);

    while (!diagonals2.empty()) {
      diagonal = *(diagonals2.begin());
      diagonals2.pop_front();
      if ((diagonal.index2 - diagonal.index1) <= 1) {
        continue;
      }
      ijreal = true;
      jkreal = true;
      pairs = &(dpstates[diagonal.index1][diagonal.index2].pairs);
      if (!vertices[diagonal.index1].isConvex) {
        iter = pairs->end();
        --iter;
        j = iter->index2;
        if (iter->index1 != iter->index2) {
          ijreal = false;
        }
      } else {
        iter = pairs->begin();
        j = iter->index1;
        if (iter->index1 != iter->index2) {
          jkreal = false;
        }
      }

      if (ijreal) {
        diagonals.emplace_back(diagonal.index1, j);
      } else {
        diagonals2.emplace_back(diagonal.index1, j);
      }

      if (jkreal) {
        diagonals.emplace_back(j, diagonal.index2);
      } else {
        diagonals2.emplace_back(j, diagonal.index2);
      }

      indices.push_back(j);
    }

    std::sort(indices.begin(), indices.end());
    newpoly.Init((tppl_idx)indices.size());
    k = 0;
    for (iiter = indices.begin(); iiter != indices.end(); ++iiter) {
      newpoly[k] = vertices[*iiter].p;
      k++;
    }
    parts->push_back(newpoly);
  }

  for (i = 0; i < n; i++) {
    delete[] dpstates[i];
  }
  delete[] dpstates;
  delete[] vertices;

  return ret;
}

// Creates a monotone partition of a list of polygons that
// can contain holes. Triangulates a set of polygons by
// first partitioning them into monotone polygons.
// Time complexity: O(n*log(n)), n is the number of vertices.
// Space complexity: O(n)
// The algorithm used here is outlined in the book
// "Computational Geometry: Algorithms and Applications"
// by Mark de Berg, Otfried Cheong, Marc van Kreveld, and Mark Overmars.
int TPPLPartition::MonotonePartition(TPPLPolyList *inpolys, TPPLPolyList *monotonePolys) {
  TPPLPolyList::iterator iter;
  MonotoneVertex *vertices = nullptr;
  tppl_idx i, numvertices, vindex, vindex2, newnumvertices, maxnumvertices;
  tppl_idx polystartindex, polyendindex;
  TPPLPoly *poly = nullptr;
  MonotoneVertex *v = nullptr, *v2 = nullptr, *vprev = nullptr, *vnext = nullptr;
  ScanLineEdge newedge;
  bool error = false;

  numvertices = 0;
  for (iter = inpolys->begin(); iter != inpolys->end(); ++iter) {
    if (!iter->Valid()) {
      return 0;
    }
    numvertices += iter->GetNumPoints();
  }

  maxnumvertices = numvertices * 3;
  vertices = new MonotoneVertex[maxnumvertices];
  newnumvertices = numvertices;

  polystartindex = 0;
  for (iter = inpolys->begin(); iter != inpolys->end(); ++iter) {
    poly = &(*iter);
    polyendindex = polystartindex + poly->GetNumPoints() - 1;
    for (i = 0; i < poly->GetNumPoints(); i++) {
      vertices[i + polystartindex].p = poly->GetPoint(i);
      if (i == 0) {
        vertices[i + polystartindex].previous = polyendindex;
      } else {
        vertices[i + polystartindex].previous = i + polystartindex - 1;
      }
      if (i == (poly->GetNumPoints() - 1)) {
        vertices[i + polystartindex].next = polystartindex;
      } else {
        vertices[i + polystartindex].next = i + polystartindex + 1;
      }
    }
    polystartindex = polyendindex + 1;
  }

  // Construct the priority queue.
  auto priority = new tppl_idx[numvertices];
  for (i = 0; i < numvertices; i++) {
    priority[i] = i;
  }
  std::sort(priority, &(priority[numvertices]), VertexSorter(vertices));

  // Determine vertex types.
  auto vertextypes = new TPPLVertexType[maxnumvertices];
  for (i = 0; i < numvertices; i++) {
    v = &(vertices[i]);
    vprev = &(vertices[v->previous]);
    vnext = &(vertices[v->next]);

    if (Below(vprev->p, v->p) && Below(vnext->p, v->p)) {
      if (IsConvex(vnext->p, vprev->p, v->p)) {
        vertextypes[i] = TPPL_VERTEXTYPE_START;
      } else {
        vertextypes[i] = TPPL_VERTEXTYPE_SPLIT;
      }
    } else if (Below(v->p, vprev->p) && Below(v->p, vnext->p)) {
      if (IsConvex(vnext->p, vprev->p, v->p)) {
        vertextypes[i] = TPPL_VERTEXTYPE_END;
      } else {
        vertextypes[i] = TPPL_VERTEXTYPE_MERGE;
      }
    } else {
      vertextypes[i] = TPPL_VERTEXTYPE_REGULAR;
    }
  }

  // Helpers.
  auto helpers = new tppl_idx[maxnumvertices];

  // Binary search tree that holds edges intersecting the scanline.
  // Note that while set doesn't actually have to be implemented as
  // a tree, complexity requirements for operations are the same as
  // for the balanced binary search tree.
  std::set<ScanLineEdge> edgeTree;
  // Store iterators to the edge tree elements.
  // This makes deleting existing edges much faster.
  std::set<ScanLineEdge>::iterator *edgeTreeIterators, edgeIter;
  edgeTreeIterators = new std::set<ScanLineEdge>::iterator[maxnumvertices];
  std::pair<std::set<ScanLineEdge>::iterator, bool> edgeTreeRet;
  for (i = 0; i < numvertices; i++) {
    edgeTreeIterators[i] = edgeTree.end();
  }

  // For each vertex.
  for (i = 0; i < numvertices; i++) {
    vindex = priority[i];
    v = &(vertices[vindex]);
    vindex2 = vindex;
    v2 = v;

    // Depending on the vertex type, do the appropriate action.
    // Comments in the following sections are copied from
    // "Computational Geometry: Algorithms and Applications".
    // Notation: e_i = e subscript i, v_i = v subscript i, etc.
    switch (vertextypes[vindex]) {
      case TPPL_VERTEXTYPE_START:
        // Insert e_i in T and set helper(e_i) to v_i.
        newedge.p1 = v->p;
        newedge.p2 = vertices[v->next].p;
        newedge.index = vindex;
        edgeTreeRet = edgeTree.insert(newedge);
        edgeTreeIterators[vindex] = edgeTreeRet.first;
        helpers[vindex] = vindex;
        break;

      case TPPL_VERTEXTYPE_END:
        if (edgeTreeIterators[v->previous] == edgeTree.end()) {
          error = true;
          break;
        }
        // If helper(e_i - 1) is a merge vertex
        if (vertextypes[helpers[v->previous]] == TPPL_VERTEXTYPE_MERGE) {
          // Insert the diagonal connecting vi to helper(e_i - 1) in D.
          AddDiagonal(vertices, &newnumvertices, vindex, helpers[v->previous],
                  vertextypes, edgeTreeIterators, &edgeTree, helpers);
        }
        // Delete e_i - 1 from T
        edgeTree.erase(edgeTreeIterators[v->previous]);
        break;

      case TPPL_VERTEXTYPE_SPLIT:
        // Search in T to find the edge e_j directly left of v_i.
        newedge.p1 = v->p;
        newedge.p2 = v->p;
        edgeIter = edgeTree.lower_bound(newedge);
        if (edgeIter == edgeTree.begin()) {
          error = true;
          break;
        }
        --edgeIter;
        // Insert the diagonal connecting vi to helper(e_j) in D.
        AddDiagonal(vertices, &newnumvertices, vindex, helpers[edgeIter->index],
                vertextypes, edgeTreeIterators, &edgeTree, helpers);
        vindex2 = newnumvertices - 2;
        v2 = &(vertices[vindex2]);
        // helper(e_j) in v_i.
        helpers[edgeIter->index] = vindex;
        // Insert e_i in T and set helper(e_i) to v_i.
        newedge.p1 = v2->p;
        newedge.p2 = vertices[v2->next].p;
        newedge.index = vindex2;
        edgeTreeRet = edgeTree.insert(newedge);
        edgeTreeIterators[vindex2] = edgeTreeRet.first;
        helpers[vindex2] = vindex2;
        break;

      case TPPL_VERTEXTYPE_MERGE:
        if (edgeTreeIterators[v->previous] == edgeTree.end()) {
          error = true;
          break;
        }
        // if helper(e_i - 1) is a merge vertex
        if (vertextypes[helpers[v->previous]] == TPPL_VERTEXTYPE_MERGE) {
          // Insert the diagonal connecting vi to helper(e_i - 1) in D.
          AddDiagonal(vertices, &newnumvertices, vindex, helpers[v->previous],
                  vertextypes, edgeTreeIterators, &edgeTree, helpers);
          vindex2 = newnumvertices - 2;
        }
        // Delete e_i - 1 from T.
        edgeTree.erase(edgeTreeIterators[v->previous]);
        // Search in T to find the edge e_j directly left of v_i.
        newedge.p1 = v->p;
        newedge.p2 = v->p;
        edgeIter = edgeTree.lower_bound(newedge);
        if (edgeIter == edgeTree.begin()) {
          error = true;
          break;
        }
        --edgeIter;
        // If helper(e_j) is a merge vertex.
        if (vertextypes[helpers[edgeIter->index]] == TPPL_VERTEXTYPE_MERGE) {
          // Insert the diagonal connecting v_i to helper(e_j) in D.
          AddDiagonal(vertices, &newnumvertices, vindex2, helpers[edgeIter->index],
                  vertextypes, edgeTreeIterators, &edgeTree, helpers);
        }
        // helper(e_j) <- v_i
        helpers[edgeIter->index] = vindex2;
        break;

      case TPPL_VERTEXTYPE_REGULAR:
        // If the interior of P lies to the right of v_i.
        if (Below(v->p, vertices[v->previous].p)) {
          if (edgeTreeIterators[v->previous] == edgeTree.end()) {
            error = true;
            break;
          }
          // If helper(e_i - 1) is a merge vertex.
          if (vertextypes[helpers[v->previous]] == TPPL_VERTEXTYPE_MERGE) {
            // Insert the diagonal connecting v_i to helper(e_i - 1) in D.
            AddDiagonal(vertices, &newnumvertices, vindex, helpers[v->previous],
                    vertextypes, edgeTreeIterators, &edgeTree, helpers);
            vindex2 = newnumvertices - 2;
            v2 = &(vertices[vindex2]);
          }
          // Delete e_i - 1 from T.
          edgeTree.erase(edgeTreeIterators[v->previous]);
          // Insert e_i in T and set helper(e_i) to v_i.
          newedge.p1 = v2->p;
          newedge.p2 = vertices[v2->next].p;
          newedge.index = vindex2;
          edgeTreeRet = edgeTree.insert(newedge);
          edgeTreeIterators[vindex2] = edgeTreeRet.first;
          helpers[vindex2] = vindex;
        } else {
          // Search in T to find the edge e_j directly left of v_i.
          newedge.p1 = v->p;
          newedge.p2 = v->p;
          edgeIter = edgeTree.lower_bound(newedge);
          if (edgeIter == edgeTree.begin()) {
            error = true;
            break;
          }
          --edgeIter;
          // If helper(e_j) is a merge vertex.
          if (vertextypes[helpers[edgeIter->index]] == TPPL_VERTEXTYPE_MERGE) {
            // Insert the diagonal connecting v_i to helper(e_j) in D.
            AddDiagonal(vertices, &newnumvertices, vindex, helpers[edgeIter->index],
                    vertextypes, edgeTreeIterators, &edgeTree, helpers);
          }
          // helper(e_j) <- v_i.
          helpers[edgeIter->index] = vindex;
        }
        break;
    }

    if (error)
      break;
  }

  auto used = new char[newnumvertices];
  memset(used, 0, newnumvertices * sizeof(char));

  if (!error) {
    // Return result.
    tppl_idx size;
    TPPLPoly mpoly;
    for (i = 0; i < newnumvertices; i++) {
      if (used[i]) {
        continue;
      }
      v = &(vertices[i]);
      vnext = &(vertices[v->next]);
      size = 1;
      while (vnext != v) {
        vnext = &(vertices[vnext->next]);
        size++;
      }
      mpoly.Init(size);
      v = &(vertices[i]);
      mpoly[0] = v->p;
      vnext = &(vertices[v->next]);
      size = 1;
      used[i] = 1;
      used[v->next] = 1;
      while (vnext != v) {
        mpoly[size] = vnext->p;
        used[vnext->next] = 1;
        vnext = &(vertices[vnext->next]);
        size++;
      }
      monotonePolys->push_back(mpoly);
    }
  }

  // Cleanup.
  delete[] vertices;
  delete[] priority;
  delete[] vertextypes;
  delete[] edgeTreeIterators;
  delete[] helpers;
  delete[] used;

  if (error) {
    return 0;
  }
  return 1;
}

// Adds a diagonal to the doubly-connected list of vertices.
void TPPLPartition::AddDiagonal(MonotoneVertex *vertices, tppl_idx *numvertices, const tppl_idx index1, const tppl_idx index2,
        TPPLVertexType *vertextypes, std::set<ScanLineEdge>::iterator *edgeTreeIterators,
        std::set<ScanLineEdge> *edgeTree, tppl_idx *helpers) {
  tppl_idx newindex1, newindex2;

  newindex1 = *numvertices;
  (*numvertices)++;
  newindex2 = *numvertices;
  (*numvertices)++;

  vertices[newindex1].p = vertices[index1].p;
  vertices[newindex2].p = vertices[index2].p;

  vertices[newindex2].next = vertices[index2].next;
  vertices[newindex1].next = vertices[index1].next;

  vertices[vertices[index2].next].previous = newindex2;
  vertices[vertices[index1].next].previous = newindex1;

  vertices[index1].next = newindex2;
  vertices[newindex2].previous = index1;

  vertices[index2].next = newindex1;
  vertices[newindex1].previous = index2;

  // Update all relevant structures.
  vertextypes[newindex1] = vertextypes[index1];
  edgeTreeIterators[newindex1] = edgeTreeIterators[index1];
  helpers[newindex1] = helpers[index1];
  if (edgeTreeIterators[newindex1] != edgeTree->end()) {
    edgeTreeIterators[newindex1]->index = newindex1;
  }
  vertextypes[newindex2] = vertextypes[index2];
  edgeTreeIterators[newindex2] = edgeTreeIterators[index2];
  helpers[newindex2] = helpers[index2];
  if (edgeTreeIterators[newindex2] != edgeTree->end()) {
    edgeTreeIterators[newindex2]->index = newindex2;
  }
}

bool TPPLPartition::Below(const TPPLPoint &p1, const TPPLPoint &p2) {
  if (p1.y < p2.y) {
    return true;
  }
  return p1.y == p2.y && p1.x < p2.x;
}

// Sorts in the falling order of y values, if y is equal, x is used instead.
bool TPPLPartition::VertexSorter::operator()(const tppl_idx index1, const tppl_idx index2) const {
  if (vertices[index1].p.y > vertices[index2].p.y) {
    return true;
  }
  return vertices[index1].p.y == vertices[index2].p.y && vertices[index1].p.x > vertices[index2].p.x;
}

bool TPPLPartition::ScanLineEdge::IsConvex(const TPPLPoint &p1, const TPPLPoint &p2, const TPPLPoint &p3) const {
  const tppl_float tmp = (p3.y - p1.y) * (p2.x - p1.x) - (p3.x - p1.x) * (p2.y - p1.y);
  return tmp > 0;
}

bool TPPLPartition::ScanLineEdge::operator<(const ScanLineEdge &other) const {
  if (other.p1.y == other.p2.y) {
    if (p1.y == p2.y) {
      return (p1.y < other.p1.y);
    }
    return IsConvex(p1, p2, other.p1);
  }
  if (p1.y == p2.y) {
    return !IsConvex(other.p1, other.p2, p1);
  }
  if (p1.y < other.p1.y) {
    return !IsConvex(other.p1, other.p2, p1);
  }
  return IsConvex(p1, p2, other.p1);
}

// Triangulates monotone polygon.
// Time complexity: O(n)
// Space complexity: O(n)
int TPPLPartition::TriangulateMonotone(const TPPLPoly *inPoly, TPPLPolyList *triangles) {
  if (!inPoly->Valid()) {
    return 0;
  }

  tppl_idx i, i2, j, topindex, bottomindex, leftindex, rightindex, vindex;
  tppl_idx numpoints;
  TPPLPoly triangle;

  numpoints = inPoly->GetNumPoints();
  auto const& points = inPoly->GetPoints();

  // Trivial case.
  if (numpoints == 3) {
    triangles->push_back(*inPoly);
    return 1;
  }

  topindex = 0;
  bottomindex = 0;
  for (i = 1; i < numpoints; i++) {
    if (Below(points[i], points[bottomindex])) {
      bottomindex = i;
    }
    if (Below(points[topindex], points[i])) {
      topindex = i;
    }
  }

  // Check if the poly is really monotone.
  i = topindex;
  while (i != bottomindex) {
    i2 = i + 1;
    if (i2 >= numpoints) {
      i2 = 0;
    }
    if (!Below(points[i2], points[i])) {
      return 0;
    }
    i = i2;
  }
  i = bottomindex;
  while (i != topindex) {
    i2 = i + 1;
    if (i2 >= numpoints) {
      i2 = 0;
    }
    if (!Below(points[i], points[i2])) {
      return 0;
    }
    i = i2;
  }

  auto vertextypes = new char[numpoints];
  auto priority = new tppl_idx[numpoints];

  // Merge left and right vertex chains.
  priority[0] = topindex;
  vertextypes[topindex] = 0;
  leftindex = topindex + 1;
  if (leftindex >= numpoints) {
    leftindex = 0;
  }
  rightindex = topindex - 1;
  if (rightindex < 0) {
    rightindex = numpoints - 1;
  }
  for (i = 1; i < (numpoints - 1); i++) {
    if (leftindex == bottomindex) {
      priority[i] = rightindex;
      rightindex--;
      if (rightindex < 0) {
        rightindex = numpoints - 1;
      }
      vertextypes[priority[i]] = -1;
    } else if (rightindex == bottomindex) {
      priority[i] = leftindex;
      leftindex++;
      if (leftindex >= numpoints) {
        leftindex = 0;
      }
      vertextypes[priority[i]] = 1;
    } else {
      if (Below(points[leftindex], points[rightindex])) {
        priority[i] = rightindex;
        rightindex--;
        if (rightindex < 0) {
          rightindex = numpoints - 1;
        }
        vertextypes[priority[i]] = -1;
      } else {
        priority[i] = leftindex;
        leftindex++;
        if (leftindex >= numpoints) {
          leftindex = 0;
        }
        vertextypes[priority[i]] = 1;
      }
    }
  }
  priority[i] = bottomindex;
  vertextypes[bottomindex] = 0;

  auto stack = new tppl_idx[numpoints];

  stack[0] = priority[0];
  stack[1] = priority[1];
  tppl_idx stackptr = 2;

  // For each vertex from top to bottom trim as many triangles as possible.
  for (i = 2; i < (numpoints - 1); i++) {
    vindex = priority[i];
    if (vertextypes[vindex] != vertextypes[stack[stackptr - 1]]) {
      for (j = 0; j < (stackptr - 1); j++) {
        if (vertextypes[vindex] == 1) {
          triangle.Triangle(points[stack[j + 1]], points[stack[j]], points[vindex]);
        } else {
          triangle.Triangle(points[stack[j]], points[stack[j + 1]], points[vindex]);
        }
        triangles->push_back(triangle);
      }
      stack[0] = priority[i - 1];
      stack[1] = priority[i];
      stackptr = 2;
    } else {
      stackptr--;
      while (stackptr > 0) {
        if (vertextypes[vindex] == 1) {
          if (IsConvex(points[vindex], points[stack[stackptr - 1]], points[stack[stackptr]])) {
            triangle.Triangle(points[vindex], points[stack[stackptr - 1]], points[stack[stackptr]]);
            triangles->push_back(triangle);
            stackptr--;
          } else {
            break;
          }
        } else {
          if (IsConvex(points[vindex], points[stack[stackptr]], points[stack[stackptr - 1]])) {
            triangle.Triangle(points[vindex], points[stack[stackptr]], points[stack[stackptr - 1]]);
            triangles->push_back(triangle);
            stackptr--;
          } else {
            break;
          }
        }
      }
      stackptr++;
      stack[stackptr] = vindex;
      stackptr++;
    }
  }
  vindex = priority[i];
  for (j = 0; j < (stackptr - 1); j++) {
    if (vertextypes[stack[j + 1]] == 1) {
      triangle.Triangle(points[stack[j]], points[stack[j + 1]], points[vindex]);
    } else {
      triangle.Triangle(points[stack[j + 1]], points[stack[j]], points[vindex]);
    }
    triangles->push_back(triangle);
  }

  delete[] priority;
  delete[] vertextypes;
  delete[] stack;

  return 1;
}

int TPPLPartition::Triangulate_MONO(TPPLPolyList *inpolys, TPPLPolyList *triangles) {
  TPPLPolyList monotone;

  if (!MonotonePartition(inpolys, &monotone)) {
    return 0;
  }
  for (auto iter = monotone.begin(); iter != monotone.end(); ++iter) {
    if (!TriangulateMonotone(&(*iter), triangles)) {
      return 0;
    }
  }
  return 1;
}

int TPPLPartition::Triangulate_MONO(const TPPLPoly *poly, TPPLPolyList *triangles) {
  TPPLPolyList polys;
  polys.push_back(*poly);

  return Triangulate_MONO(&polys, triangles);
}

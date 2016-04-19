/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 *   This file is part of
 *       ______        _                             __ __
 *      / ____/____   (_)____ _ ____ ___   ____ _   / // /
 *     / __/  / __ \ / // __ `// __ `__ \ / __ `/  / // /_
 *    / /___ / / / // // /_/ // / / / / // /_/ /  /__  __/
 *   /_____//_/ /_//_/ \__, //_/ /_/ /_/ \__,_/     /_/.   
 *                    /____/                              
 *
 *   Copyright © 2003-2012 Brain Control, all rights reserved.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MESH_OPS_HPP
#define MESH_OPS_HPP

class eIPathOp;

// base class for mesh operators
class eIMeshOp : public eIOperator
{
public:
    struct Result : public eOpResult
    {
        eEditMesh mesh;
    };

public:
    eIMeshOp() : m_mesh(m_res.mesh)
    {
    }

    virtual const Result & getResult() const
    {
        return m_res;
    }

    virtual eU32 getResultSize() const
    {
        return m_mesh.getFaceCount()*sizeof(eEmFace)+
               m_mesh.getPositionCount()*sizeof(eEmVtxPos)+
               m_mesh.getWedgeCount()*sizeof(eEmWedge)+
               m_mesh.getPropertyCount()*sizeof(eEmVtxProps)+
               m_mesh.getEdgeCount()*sizeof(eEmEdge)+
               m_mesh.getNormalCount()*sizeof(eVector3);
    }

    virtual void freeResult()
    {
        m_mesh.free();
    }

    // used for mesh and model multiply ops
    static void processMultiply(eU32 mode, eU32 count, const eVector3 &trans, const eVector3 &rot, const eVector3 &scale,
                                eF32 radius, const eVector3 &circularRot, eF32 timeStart, eF32 timeEnd, const eVector3 &pathScale,
                                const eVector3 &upVector, const eIPathOp *pathOp, const eVector3 &randTrans, const eVector3 &randRot,
                                const eVector3 &randScale, eU32 seed, ePtr parent, eConstPtr element, void(* addElement)(ePtr, eConstPtr, const eTransform &));

protected:
    virtual void _preExecute()
    {
        m_mesh.clear();
    }

    void _copyFirstInputMesh()
    {
        m_mesh.merge(((eIMeshOp *)getAboveOp(0))->getResult().mesh);
    }

    void _selectPrimitive(eBool &selected, eInt mode)
    {
        eASSERT(mode >= 0 && mode <= 3);

        const eBool todo[4] =
        {
            eTRUE,      // set
            eTRUE,      // add
            eFALSE,     // remove
            !selected   // toggle
        };

        selected = todo[mode];
    }

protected:
    eEditMesh & m_mesh;
    Result      m_res;
};

#endif // MESH_OPS_HPP
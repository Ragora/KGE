/**
 *  @file CSceneGraph.cpp
 *  @brief The source file implementing the Kiaro::Engine::CSceneGraph class.
 *
 *  This software is licensed under the Draconic Free License version 1. Please refer
 *  to LICENSE.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <core/SEngineInstance.hpp>
#include <video/CSceneGraph.hpp>

namespace Kiaro
{
    namespace Video
    {
        CSceneGraph::CSceneGraph(void)
        {
            mRoot = Core::SEngineInstance::getPointer()->getSceneManager()->addEmptySceneNode();
        }

        CSceneGraph::~CSceneGraph(void)
        {
            mRoot->removeAll();
            mRoot->remove();
            mRoot = NULL;

            std::cout << "CSceneGraph: Destroyed a scene graph." << std::endl;
        }

        void CSceneGraph::add(irr::scene::ISceneNode *node)
        {
            mRoot->addChild(node);
        }

        bool CSceneGraph::isVisible(void)
        {
            return mRoot->isVisible();
        }

        void CSceneGraph::setVisible(const bool &visible)
        {
            mRoot->setVisible(visible);
        }

        const Common::ColorRGBA &CSceneGraph::getClearColor(void)
        {
            return mClearColor;
        }

        void CSceneGraph::setClearColor(const Common::ColorRGBA &color)
        {
            mClearColor = color;
        }
    } // End NameSpace Engine
} // End NameSpace Kiaro
// Aseprite
// Copyright (C) 2019-2020  Igara Studio S.A.
//
// This program is distributed under the terms of
// the End-User License Agreement for Aseprite.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "app/active_site_handler.h"
#include "app/doc.h"
#include "app/doc_event.h"
#include "app/site.h"
#include "app/util/layer_utils.h"
#include "doc/layer.h"

namespace app {

ActiveSiteHandler::ActiveSiteHandler()
{
}

ActiveSiteHandler::~ActiveSiteHandler()
{
}

void ActiveSiteHandler::addDoc(Doc* doc)
{
  Data data;
  if (doc::Layer* layer = doc->sprite()->root()->firstLayer())
    data.layer = layer->id();
  else
    data.layer = doc::NullId;
  data.frame = 0;
  m_data.insert(std::make_pair(doc, data));
  doc->add_observer(this);
}

void ActiveSiteHandler::removeDoc(Doc* doc)
{
  auto it = m_data.find(doc);
  if (it == m_data.end())
    return;

  doc->remove_observer(this);
  m_data.erase(it);
}

ActiveSiteHandler::Data& ActiveSiteHandler::getData(Doc* doc)
{
  auto it = m_data.find(doc);
  if (it == m_data.end()) {
    addDoc(doc);
    it = m_data.find(doc);
    ASSERT(it != m_data.end());
  }
  return it->second;
}

void ActiveSiteHandler::getActiveSiteForDoc(Doc* doc, Site* site)
{
  Data& data = getData(doc);
  site->document(doc);
  site->sprite(doc->sprite());
  site->layer(doc::get<doc::Layer>(data.layer));
  site->frame(data.frame);
  site->selectedColors(data.selectedColors);
}

void ActiveSiteHandler::setActiveLayerInDoc(Doc* doc, doc::Layer* layer)
{
  Data& data = getData(doc);
  data.layer = (layer ? layer->id(): 0);
}

void ActiveSiteHandler::setActiveFrameInDoc(Doc* doc, doc::frame_t frame)
{
  Data& data = getData(doc);
  data.frame = frame;
}

void ActiveSiteHandler::setSelectedColorsInDoc(Doc* doc, const doc::PalettePicks& picks)
{
  Data& data = getData(doc);
  data.selectedColors = picks;
}

void ActiveSiteHandler::onAddLayer(DocEvent& ev)
{
  Data& data = getData(ev.document());
  data.layer = ev.layer()->id();
}

void ActiveSiteHandler::onAddFrame(DocEvent& ev)
{
  Data& data = getData(ev.document());
  data.frame = ev.frame();
}

// TODO similar to Timeline::onBeforeRemoveLayer() and Editor::onBeforeRemoveLayer()
void ActiveSiteHandler::onBeforeRemoveLayer(DocEvent& ev)
{
  Data& data = getData(ev.document());
  doc::Layer* selectedLayer = (data.layer != doc::NullId ?
                               doc::get<doc::Layer>(data.layer):
                               nullptr);
  if (!selectedLayer)
    return;

  doc::Layer* layerToSelect = candidate_if_layer_is_deleted(selectedLayer, ev.layer());
  if (selectedLayer != layerToSelect) {
    data.layer = (layerToSelect ? layerToSelect->id():
                                  doc::NullId);
  }
}

// TODO similar to Timeline::onRemoveFrame()
void ActiveSiteHandler::onRemoveFrame(DocEvent& ev)
{
  Data& data = getData(ev.document());

  // Adjust current frame of the data that are in a frame more
  // advanced that the removed one.
  if (data.frame > ev.frame()) {
    --data.frame;
  }
  // If the data was in the previous "last frame" (current value of
  // totalFrames()), we've to adjust it to the new last frame
  // (lastFrame())
  else if (data.frame >= ev.sprite()->totalFrames()) {
    data.frame = ev.sprite()->lastFrame();
  }

  if (data.frame < ev.frame())
    --data.frame;
}

} // namespace app

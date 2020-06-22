#include "EventListener.h"

EventListener::EventListener(GameObject* gameObject)
    : gameObject_(gameObject) {}

GameObject* EventListener::getGameObject() const { return gameObject_; }

bool EventListener::getActive() { return active_; }

void EventListener::setActive(bool active) { active_ = active; }
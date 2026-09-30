#pragma once

#include "annotation_item.h"
#include <QUndoStack>
#include <QUndoCommand>
#include <QList>
#include <memory>

class AddAnnotationCommand : public QUndoCommand {
public:
    AddAnnotationCommand(QList<std::shared_ptr<AnnotationItem>>& list,
                         std::shared_ptr<AnnotationItem> item,
                         QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_list(list), m_item(std::move(item)) {}

    void redo() override {
        m_list.append(m_item);
    }

    void undo() override {
        if (!m_list.isEmpty() && m_list.last() == m_item) {
            m_list.removeLast();
        } else {
            m_list.removeAll(m_item);
        }
    }

private:
    QList<std::shared_ptr<AnnotationItem>>& m_list;
    std::shared_ptr<AnnotationItem> m_item;
};

class AnnotationManager {
public:
    AnnotationManager() = default;
    ~AnnotationManager() = default;

    void addItem(std::shared_ptr<AnnotationItem> item) {
        m_undoStack.push(new AddAnnotationCommand(m_items, std::move(item)));
    }

    void undo() {
        if (m_undoStack.canUndo()) {
            m_undoStack.undo();
        }
    }

    void redo() {
        if (m_undoStack.canRedo()) {
            m_undoStack.redo();
        }
    }

    void clear() {
        m_undoStack.clear();
        m_items.clear();
    }

    const QList<std::shared_ptr<AnnotationItem>>& items() const {
        return m_items;
    }

    void renderAnnotations(QPainter& painter) const {
        for (const auto& item : m_items) {
            item->draw(painter);
        }
    }

private:
    QList<std::shared_ptr<AnnotationItem>> m_items;
    QUndoStack m_undoStack;
};

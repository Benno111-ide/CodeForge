#pragma once

#include <string>
#include <vector>
#include <memory>
#include "imgui.h"

class CodeEditor {
public:
    struct Position {
        int line;
        int column;
        
        Position() : line(0), column(0) {}
        Position(int l, int c) : line(l), column(c) {}
        
        bool operator==(const Position& other) const {
            return line == other.line && column == other.column;
        }
        
        bool operator!=(const Position& other) const {
            return !(*this == other);
        }
    };

    struct Selection {
        Position start;
        Position end;
        Position anchor;
        bool active = false;
        
        void clear() { active = false; start = end = anchor = Position(0, 0); }
        void set(Position s, Position e) {
            start = s;
            end = e;
            anchor = s;
            active = true;
            normalize();
        }
        
        void normalize() {
            if (start.line > end.line || (start.line == end.line && start.column > end.column)) {
                std::swap(start, end);
            }
        }
    };

    CodeEditor();
    ~CodeEditor();

    // Core interface
    void render(const char* title, bool* open = nullptr);
    void setText(const std::string& text);
    std::string getText() const;
    void clear();
    
    // Configuration
    void setSyntaxHighlighting(bool enabled) { syntaxHighlighting = enabled; }
    void setLineNumbers(bool enabled) { lineNumbers = enabled; }
    void setReadOnly(bool readOnly) { this->readOnly = readOnly; }
    void setAutoScroll(bool enabled) { autoScrollCursor = enabled; }
    bool loadFile(const std::string& path);
    bool saveFile(const std::string& path);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    
    // State queries
    bool isModified() const { return modified; }
    Position getCursorPosition() const { return cursor; }
    bool hasSelection() const { return selection.active; }

private:
    // Core rendering functions
    void renderEditor();
    void renderLineNumbers(ImDrawList* drawList, ImVec2 startPos, float lineHeight, int firstLine, int lastLine);
    void renderText(ImDrawList* drawList, ImVec2 startPos, float lineHeight, int firstLine, int lastLine);
    void renderHighlightedLine(ImDrawList* drawList, const std::string& line, ImVec2 textPos);
    void renderCursor(ImDrawList* drawList, ImVec2 startPos, float lineHeight);
    void renderSelection(ImDrawList* drawList, ImVec2 startPos, float lineHeight);
    
    // Input handling
    void handleKeyboard();
    void handleMouse(ImVec2 startPos, float lineHeight);
    
    // Text operations
    void insertText(const std::string& text);
    void deleteSelection();
    char deleteChar(); // Returns deleted character
    void insertChar(char c);
    void insertNewline();
    
    // Navigation
    void moveCursor(int deltaLine, int deltaColumn, bool extendSelection = false);
    void setCursor(Position pos, bool extendSelection = false);
    Position screenToTextPos(ImVec2 screenPos, ImVec2 textStart, float lineHeight) const;
    ImVec2 textToScreenPos(Position pos, ImVec2 textStart, float lineHeight) const;
    Position wordBoundaryLeft(Position from) const;
    Position wordBoundaryRight(Position from) const;
    
    // Scrolling
    void updateScroll();
    void ensureCursorVisible();
    int getVisibleLineCount() const;
    
    // Utility
    void splitTextIntoLines();
    void rebuildText();
    float getMaxLineWidth() const;
    std::string buildTextFromLines() const;
    std::string getSelectedText() const;
    
    struct EditorState {
        std::vector<std::string> linesSnapshot;
        Position cursorSnapshot;
        Selection selectionSnapshot;
        bool modifiedSnapshot = false;
    };
    
    class HistoryScope {
    public:
        explicit HistoryScope(CodeEditor& editor)
            : editor(editor), locked(editor.beginHistoryEntry()) {}
        ~HistoryScope() { editor.endHistoryEntry(locked); }
    private:
        CodeEditor& editor;
        bool locked = false;
    };
    
    void pushUndoState();
    void restoreState(const EditorState& state);
    bool beginHistoryEntry();
    void endHistoryEntry(bool locked);
    
    // State
    std::string text;
    std::vector<std::string> lines;
    Position cursor;
    Selection selection;
    std::vector<EditorState> undoStack;
    std::vector<EditorState> redoStack;
    
    // View state
    float scrollY = 0.0f;
    float scrollX = 0.0f;
    
    // Configuration
    bool syntaxHighlighting = true;
    bool lineNumbers = true;
    bool readOnly = false;
    bool modified = false;
    int tabSize = 4;
    bool autoScrollCursor = true;
    
    // UI state
    bool focused = false;
    float blinkTime = 0.0f;
    bool historyLocked = false;
    static constexpr size_t maxHistoryStates = 128;
    
    // Cache
    mutable float maxLineWidth = -1.0f; // -1 means needs recalculation
};

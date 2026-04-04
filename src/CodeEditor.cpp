#include "../include/CodeEditor.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

CodeEditor::CodeEditor() {
    lines.emplace_back(""); // Start with one empty line
}

CodeEditor::~CodeEditor() = default;

void CodeEditor::render(const char* title, bool* open) {
    if (!ImGui::Begin(title, open)) {
        ImGui::End();
        return;
    }
    renderEditor();
    
    ImGui::End();
}

void CodeEditor::renderEditor() {
    ImVec2 contentRegion = ImGui::GetContentRegionAvail();
    contentRegion.y -= ImGui::GetTextLineHeightWithSpacing(); // Leave space for status
    
    float lineHeight = ImGui::GetTextLineHeight();
    float lineNumberWidth = lineNumbers ? ImGui::CalcTextSize("9999").x + 20.0f : 0.0f;
    
    // Create scrollable child window
    ImGui::BeginChild("##editor_content", contentRegion, true, 
                     ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);
    
    // Track focus on the actual child so keyboard input works when the user clicks inside the editor area
    focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    
    // Track current scroll from ImGui
    scrollY = ImGui::GetScrollY();
    scrollX = ImGui::GetScrollX();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 contentStart = ImGui::GetCursorScreenPos();
    ImVec2 textStart(contentStart.x + lineNumberWidth, contentStart.y);
    
    // Clip to visible lines via ImGuiListClipper to avoid manual scroll math
    ImGuiListClipper clipper;
    clipper.Begin((int)lines.size(), lineHeight);
    while (clipper.Step()) {
        if (lineNumbers) {
            renderLineNumbers(drawList, contentStart, lineHeight, clipper.DisplayStart, clipper.DisplayEnd - 1);
        }
        renderText(drawList, textStart, lineHeight, clipper.DisplayStart, clipper.DisplayEnd - 1);
    }
    
    renderSelection(drawList, textStart, lineHeight);
    renderCursor(drawList, textStart, lineHeight);
    
    // Reserve size so ImGui knows scroll extents
    float totalHeight = lines.size() * lineHeight;
    // Base the scrollable width solely on the actual text width so lines stay full length regardless of viewport size
    float maxWidth = getMaxLineWidth() + lineNumberWidth + 20.0f;
    ImGui::Dummy(ImVec2(maxWidth, totalHeight));
    
    // Handle input
    handleKeyboard();
    handleMouse(textStart, lineHeight);
    
    // Update scroll if needed
    updateScroll();
    
    ImGui::EndChild();
    
    // Status bar
    ImGui::Text("Line %d, Column %d | %s", 
               cursor.line + 1, cursor.column + 1, 
               modified ? "Modified" : "Saved");
}

void CodeEditor::renderLineNumbers(ImDrawList* drawList, ImVec2 startPos, float lineHeight, int firstLine, int lastLine) {
    ImU32 lineNumColor = ImGui::GetColorU32(ImVec4(0.6f, 0.6f, 0.7f, 1.0f));
    ImU32 separatorColor = ImGui::GetColorU32(ImVec4(0.4f, 0.4f, 0.5f, 1.0f));
    
    float lineNumberWidth = ImGui::CalcTextSize("9999").x + 20.0f;
    float yOffset = -scrollY + (firstLine * lineHeight);
    
    for (int i = firstLine; i <= lastLine; ++i) {
        float lineY = startPos.y + yOffset + ((i - firstLine) * lineHeight);
        
        char lineNum[16];
        snprintf(lineNum, sizeof(lineNum), "%4d", i + 1);
        
        ImVec2 lineNumPos(startPos.x + 5.0f, lineY);
        drawList->AddText(lineNumPos, lineNumColor, lineNum);
    }
    
    // Draw separator line
    float sepX = startPos.x + lineNumberWidth - 10.0f;
    ImVec2 sepStart(sepX, startPos.y);
    ImVec2 sepEnd(sepX, startPos.y + ImGui::GetContentRegionAvail().y);
    drawList->AddLine(sepStart, sepEnd, separatorColor);
}

void CodeEditor::renderText(ImDrawList* drawList, ImVec2 startPos, float lineHeight, int firstLine, int lastLine) {
    ImU32 textColor = ImGui::GetColorU32(ImGuiCol_Text);
    float yOffset = -scrollY + (firstLine * lineHeight);
    
    for (int i = firstLine; i <= lastLine && i < (int)lines.size(); ++i) {
        float lineY = startPos.y + yOffset + ((i - firstLine) * lineHeight);
        ImVec2 textPos(startPos.x - scrollX, lineY);
        if (!lines[i].empty()) {
            if (syntaxHighlighting) {
                renderHighlightedLine(drawList, lines[i], textPos);
            } else {
                drawList->AddText(textPos, textColor, lines[i].c_str());
            }
        }
    }
}

void CodeEditor::renderCursor(ImDrawList* drawList, ImVec2 startPos, float lineHeight) {
    if (!focused || cursor.line >= (int)lines.size()) return;
    
    // Blink cursor
    blinkTime += ImGui::GetIO().DeltaTime;
    bool showCursor = fmod(blinkTime, 1.0f) < 0.5f;
    if (!showCursor) return;
    
    // Calculate cursor position
    float cursorX = startPos.x - scrollX;
    if (cursor.column > 0 && cursor.line < (int)lines.size()) {
        std::string textToCursor = lines[cursor.line].substr(0, 
                                   std::min(cursor.column, (int)lines[cursor.line].length()));
        cursorX += ImGui::CalcTextSize(textToCursor.c_str()).x;
    }
    
    float cursorY = startPos.y - scrollY + (cursor.line * lineHeight);
    
    // Draw cursor line
    ImU32 cursorColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.8f));
    ImVec2 cursorStart(cursorX, cursorY);
    ImVec2 cursorEnd(cursorX, cursorY + lineHeight);
    drawList->AddLine(cursorStart, cursorEnd, cursorColor, 2.0f);
}

void CodeEditor::renderSelection(ImDrawList* drawList, ImVec2 startPos, float lineHeight) {
    if (!selection.active) return;
    
    ImU32 selectionColor = ImGui::GetColorU32(ImVec4(0.3f, 0.5f, 0.8f, 0.3f));
    
    for (int line = selection.start.line; line <= selection.end.line; ++line) {
        if (line >= (int)lines.size()) break;
        
        int startCol = (line == selection.start.line) ? selection.start.column : 0;
        int endCol = (line == selection.end.line) ? selection.end.column : (int)lines[line].length();
        
        if (startCol >= endCol) continue;
        
        float lineY = startPos.y - scrollY + (line * lineHeight);
        
        float startX = startPos.x - scrollX;
        if (startCol > 0) {
            std::string textToStart = lines[line].substr(0, startCol);
            startX += ImGui::CalcTextSize(textToStart.c_str()).x;
        }
        
        float endX = startPos.x - scrollX;
        if (endCol > 0) {
            std::string textToEnd = lines[line].substr(0, endCol);
            endX += ImGui::CalcTextSize(textToEnd.c_str()).x;
        }
        
        ImVec2 selStart(startX, lineY);
        ImVec2 selEnd(endX, lineY + lineHeight);
        drawList->AddRectFilled(selStart, selEnd, selectionColor);
    }
}

void CodeEditor::handleKeyboard() {
    if (!focused || readOnly) return;
    
    ImGuiIO& io = ImGui::GetIO();
    bool ctrl = io.KeyCtrl;
    bool shift = io.KeyShift;
    
    // Text input
    for (int i = 0; i < io.InputQueueCharacters.Size; i++) {
        ImWchar c = io.InputQueueCharacters[i];
        if (c > 0) {
            if (c >= 32 || c == '\t') { // Printable characters and tab
                insertChar(static_cast<char>(c));
            }
        }
    }
    io.InputQueueCharacters.clear();
    
    // Navigation
    if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow)) {
        if (ctrl) {
            setCursor(wordBoundaryLeft(cursor), shift);
        } else {
            moveCursor(0, -1, shift);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_RightArrow)) {
        if (ctrl) {
            setCursor(wordBoundaryRight(cursor), shift);
        } else {
            moveCursor(0, 1, shift);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) {
        moveCursor(-1, 0, shift);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) {
        moveCursor(1, 0, shift);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home)) {
        if (ctrl) {
            setCursor(Position(0, 0), shift);
        } else {
            setCursor(Position(cursor.line, 0), shift);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_End)) {
        if (ctrl && !lines.empty()) {
            setCursor(Position((int)lines.size() - 1, (int)lines.back().length()), shift);
        } else if (cursor.line < (int)lines.size()) {
            setCursor(Position(cursor.line, (int)lines[cursor.line].length()), shift);
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageUp)) {
        int delta = -std::max(1, getVisibleLineCount() - 1);
        moveCursor(delta, 0, shift);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_PageDown)) {
        int delta = std::max(1, getVisibleLineCount() - 1);
        moveCursor(delta, 0, shift);
    }
    
    // Editing
    if (ImGui::IsKeyPressed(ImGuiKey_Backspace)) {
        if (selection.active) {
            deleteSelection();
        } else {
            deleteChar();
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Delete)) {
        if (selection.active) {
            deleteSelection();
        } else {
            // Delete forward
            moveCursor(0, 1);
            deleteChar();
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
        if (selection.active) {
            deleteSelection();
        }
        insertNewline();
    }
    
    // Selection
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_A)) {
        selectAll();
    }
    
    // Clipboard
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        copy();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_X)) {
        cut();
    }
    if (ctrl && ImGui::IsKeyPressed(ImGuiKey_V)) {
        paste();
    }
}

void CodeEditor::handleMouse(ImVec2 startPos, float lineHeight) {
    ImGuiIO& io = ImGui::GetIO();
    
    if (ImGui::IsWindowHovered()) {
        // Left click
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            Position clickPos = screenToTextPos(io.MousePos, startPos, lineHeight);
            setCursor(clickPos, io.KeyShift);
            blinkTime = 0.0f; // Reset blink
        }
        
        // Dragging
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            Position dragPos = screenToTextPos(io.MousePos, startPos, lineHeight);
            setCursor(dragPos, true); // Always extend selection when dragging
        }
    }
}

void CodeEditor::insertText(const std::string& text) {
    if (readOnly) return;
    HistoryScope history(*this);
    
    if (selection.active) {
        deleteSelection();
    }
    
    std::stringstream ss(text);
    std::string line;
    bool first = true;
    
    while (std::getline(ss, line)) {
        if (!first) {
            insertNewline();
        }
        
        for (char c : line) {
            insertChar(c);
        }
        first = false;
    }
}

void CodeEditor::deleteSelection() {
    if (!selection.active || readOnly) return;
    HistoryScope history(*this);
    
    Position start = selection.start;
    Position end = selection.end;
    start.line = std::clamp(start.line, 0, (int)lines.size() - 1);
    end.line = std::clamp(end.line, 0, (int)lines.size() - 1);
    
    if (start.line == end.line) {
        std::string& line = lines[start.line];
        int startCol = std::clamp(start.column, 0, (int)line.length());
        int endCol = std::clamp(end.column, 0, (int)line.length());
        if (endCol > startCol) {
            line.erase(startCol, endCol - startCol);
        }
    } else {
        std::string suffix = lines[end.line].substr(std::clamp(end.column, 0, (int)lines[end.line].length()));
        std::string& firstLine = lines[start.line];
        firstLine.erase(std::clamp(start.column, 0, (int)firstLine.length()));
        lines.erase(lines.begin() + start.line + 1, lines.begin() + end.line + 1);
        firstLine += suffix;
    }
    
    cursor = start;
    selection.clear();
    modified = true;
    maxLineWidth = -1;
    rebuildText();
}

char CodeEditor::deleteChar() {
    if (readOnly || cursor.line >= (int)lines.size()) return 0;
    if (cursor.column == 0 && cursor.line == 0 && lines.size() == 1 && lines[0].empty()) {
        return 0;
    }
    
    HistoryScope history(*this);
    char deleted = 0;
    bool changed = false;
    
    if (cursor.column > 0) {
        // Delete within line
        deleted = lines[cursor.line][cursor.column - 1];
        lines[cursor.line].erase(cursor.column - 1, 1);
        cursor.column--;
        changed = true;
    } else if (cursor.line > 0) {
        // Join with previous line
        cursor.column = (int)lines[cursor.line - 1].length();
        lines[cursor.line - 1] += lines[cursor.line];
        lines.erase(lines.begin() + cursor.line);
        cursor.line--;
        deleted = '\n';
        changed = true;
    }
    
    if (changed) {
        modified = true;
        maxLineWidth = -1; // Invalidate cache
        rebuildText();
    }
    
    return deleted;
}

void CodeEditor::insertChar(char c) {
    if (readOnly) return;
    HistoryScope history(*this);
    
    if (selection.active) {
        deleteSelection();
    }
    
    if (cursor.line >= (int)lines.size()) {
        cursor.line = (int)lines.size() - 1;
    }
    
    if (c == '\t') {
        // Insert spaces for tab
        int spaces = tabSize - (cursor.column % tabSize);
        for (int i = 0; i < spaces; ++i) {
            lines[cursor.line].insert(cursor.column, 1, ' ');
            cursor.column++;
        }
    } else {
        lines[cursor.line].insert(cursor.column, 1, c);
        cursor.column++;
    }
    
    modified = true;
    maxLineWidth = -1; // Invalidate cache
    ensureCursorVisible();
    rebuildText();
}

void CodeEditor::insertNewline() {
    if (readOnly) return;
    HistoryScope history(*this);
    
    if (selection.active) {
        deleteSelection();
    }
    
    if (cursor.line >= (int)lines.size()) {
        cursor.line = (int)lines.size() - 1;
    }
    
    // Split current line
    std::string rightPart = lines[cursor.line].substr(cursor.column);
    lines[cursor.line] = lines[cursor.line].substr(0, cursor.column);
    
    cursor.line++;
    cursor.column = 0;
    lines.insert(lines.begin() + cursor.line, rightPart);
    
    modified = true;
    maxLineWidth = -1; // Invalidate cache
    ensureCursorVisible();
    rebuildText();
}

void CodeEditor::moveCursor(int deltaLine, int deltaColumn, bool extendSelection) {
    Position newPos = cursor;
    newPos.line += deltaLine;
    newPos.column += deltaColumn;
    setCursor(newPos, extendSelection);
}

void CodeEditor::setCursor(Position pos, bool extendSelection) {
    // Clamp to valid range
    pos.line = std::max(0, std::min(pos.line, (int)lines.size() - 1));
    if (pos.line < (int)lines.size()) {
        pos.column = std::max(0, std::min(pos.column, (int)lines[pos.line].length()));
    }
    
    if (extendSelection) {
        if (!selection.active) {
            selection.anchor = cursor;
            selection.start = cursor;
        }
        selection.start = selection.anchor;
        selection.end = pos;
        selection.active = true;
        selection.normalize();
    } else {
        selection.clear();
    }
    
    cursor = pos;
    blinkTime = 0.0f; // Reset blink
    ensureCursorVisible();
}

CodeEditor::Position CodeEditor::screenToTextPos(ImVec2 screenPos, ImVec2 textStart, float lineHeight) const {
    Position result;
    
    // Calculate line
    float relativeY = screenPos.y - textStart.y + scrollY;
    result.line = std::max(0, (int)(relativeY / lineHeight));
    result.line = std::min(result.line, (int)lines.size() - 1);
    
    // Calculate column
    if (result.line < (int)lines.size()) {
        float relativeX = screenPos.x - textStart.x + scrollX;
        result.column = 0;
        
        const std::string& line = lines[result.line];
        for (size_t i = 0; i <= line.length(); ++i) {
            std::string substr = line.substr(0, i);
            float width = ImGui::CalcTextSize(substr.c_str()).x;
            if (relativeX <= width) {
                result.column = (int)i;
                break;
            }
        }
        result.column = std::min(result.column, (int)line.length());
    }
    
    return result;
}

ImVec2 CodeEditor::textToScreenPos(Position pos, ImVec2 textStart, float lineHeight) const {
    ImVec2 result = textStart;
    result.y += pos.line * lineHeight - scrollY;
    
    if (pos.line < (int)lines.size() && pos.column > 0) {
        std::string substr = lines[pos.line].substr(0, pos.column);
        result.x += ImGui::CalcTextSize(substr.c_str()).x - scrollX;
    } else {
        result.x -= scrollX;
    }
    
    return result;
}

CodeEditor::Position CodeEditor::wordBoundaryLeft(Position from) const {
    Position result = from;
    if (lines.empty()) return Position(0, 0);
    result.line = std::clamp(result.line, 0, (int)lines.size() - 1);
    result.column = std::clamp(result.column, 0, (int)lines[result.line].length());
    
    while (result.line >= 0) {
        const std::string& line = lines[result.line];
        int idx = std::min(result.column, (int)line.length());
        // Move left over whitespace first
        while (idx > 0 && std::isspace(static_cast<unsigned char>(line[idx - 1]))) --idx;
        // Then over non-whitespace
        while (idx > 0 && !std::isspace(static_cast<unsigned char>(line[idx - 1]))) --idx;
        if (idx > 0 || result.line == 0) {
            result.column = idx;
            return result;
        }
        // Go to previous line end
        result.line -= 1;
        result.column = (int)lines[result.line].length();
    }
    return Position(0, 0);
}

CodeEditor::Position CodeEditor::wordBoundaryRight(Position from) const {
    Position result = from;
    if (lines.empty()) return Position(0, 0);
    result.line = std::clamp(result.line, 0, (int)lines.size() - 1);
    result.column = std::clamp(result.column, 0, (int)lines[result.line].length());
    
    while (result.line < (int)lines.size()) {
        const std::string& line = lines[result.line];
        int len = (int)line.length();
        int idx = std::min(result.column, len);
        // Move right over whitespace first
        while (idx < len && std::isspace(static_cast<unsigned char>(line[idx]))) ++idx;
        // Then over non-whitespace
        while (idx < len && !std::isspace(static_cast<unsigned char>(line[idx]))) ++idx;
        if (idx < len || result.line == (int)lines.size() - 1) {
            result.column = idx;
            return result;
        }
        // Move to next line start
        result.line += 1;
        result.column = 0;
    }
    return Position((int)lines.size() - 1, (int)lines.back().length());
}

void CodeEditor::updateScroll() {
    // Sync with ImGui scroll
    if (ImGui::GetScrollY() != scrollY) {
        ImGui::SetScrollY(scrollY);
    }
    if (ImGui::GetScrollX() != scrollX) {
        ImGui::SetScrollX(scrollX);
    }
}

void CodeEditor::ensureCursorVisible() {
    if (!autoScrollCursor) return;
    
    float lineHeight = ImGui::GetTextLineHeight();
    float visibleHeight = ImGui::GetContentRegionAvail().y;
    
    float cursorY = cursor.line * lineHeight;
    
    // Adjust vertical scroll
    if (cursorY < scrollY) {
        scrollY = cursorY;
    } else if (cursorY + lineHeight > scrollY + visibleHeight) {
        scrollY = cursorY + lineHeight - visibleHeight;
    }
    
    // Adjust horizontal scroll
    if (cursor.line < (int)lines.size() && cursor.column > 0) {
        std::string textToCursor = lines[cursor.line].substr(0, cursor.column);
        float cursorX = ImGui::CalcTextSize(textToCursor.c_str()).x;
        float visibleWidth = ImGui::GetContentRegionAvail().x;
        
        if (lineNumbers) {
            visibleWidth -= ImGui::CalcTextSize("9999").x + 20.0f;
        }
        
        if (cursorX < scrollX) {
            scrollX = cursorX;
        } else if (cursorX > scrollX + visibleWidth - 50.0f) { // 50px margin
            scrollX = cursorX - visibleWidth + 50.0f;
        }
    }
    
    // Clamp scroll values
    scrollY = std::max(0.0f, scrollY);
    scrollX = std::max(0.0f, scrollX);
}

int CodeEditor::getVisibleLineCount() const {
    float lineHeight = ImGui::GetTextLineHeight();
    float visibleHeight = ImGui::GetContentRegionAvail().y;
    return (int)(visibleHeight / lineHeight) + 1;
}

void CodeEditor::setText(const std::string& newText) {
    text = newText;
    splitTextIntoLines();
    cursor = Position(0, 0);
    selection.clear();
    modified = false;
    maxLineWidth = -1; // Invalidate cache
    undoStack.clear();
    redoStack.clear();
    historyLocked = false;
}

std::string CodeEditor::getText() const {
    return buildTextFromLines();
}

void CodeEditor::clear() {
    if (readOnly) return;
    HistoryScope history(*this);
    lines.clear();
    lines.emplace_back("");
    cursor = Position(0, 0);
    selection.clear();
    modified = true;
    maxLineWidth = -1;
    rebuildText();
}

void CodeEditor::splitTextIntoLines() {
    lines.clear();
    
    if (text.empty()) {
        lines.emplace_back("");
        return;
    }
    
    std::stringstream ss(text);
    std::string line;
    
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    if (text.back() == '\n') {
        lines.emplace_back("");
    }
}

void CodeEditor::rebuildText() {
    text = buildTextFromLines();
}

float CodeEditor::getMaxLineWidth() const {
    if (maxLineWidth < 0.0f) {
        maxLineWidth = 0.0f;
        for (const auto& line : lines) {
            float width = ImGui::CalcTextSize(line.c_str()).x;
            maxLineWidth = std::max(maxLineWidth, width);
        }
    }
    return maxLineWidth;
}

bool CodeEditor::loadFile(const std::string& path) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    setText(buffer.str());
    modified = false;
    return true;
}

bool CodeEditor::saveFile(const std::string& path) {
    std::ofstream file(path, std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::string contents = buildTextFromLines();
    file << contents;
    if (!file.good()) {
        return false;
    }
    
    modified = false;
    return true;
}

void CodeEditor::undo() {
    if (undoStack.empty()) return;
    
    EditorState current;
    current.linesSnapshot = lines;
    current.cursorSnapshot = cursor;
    current.selectionSnapshot = selection;
    current.modifiedSnapshot = modified;
    redoStack.push_back(current);
    
    restoreState(undoStack.back());
    undoStack.pop_back();
}

void CodeEditor::redo() {
    if (redoStack.empty()) return;
    
    EditorState current;
    current.linesSnapshot = lines;
    current.cursorSnapshot = cursor;
    current.selectionSnapshot = selection;
    current.modifiedSnapshot = modified;
    undoStack.push_back(current);
    
    restoreState(redoStack.back());
    redoStack.pop_back();
}

void CodeEditor::cut() {
    if (readOnly || !selection.active) return;
    copy();
    deleteSelection();
}

void CodeEditor::copy() {
    if (!selection.active) return;
    std::string selected = getSelectedText();
    if (!selected.empty()) {
        ImGui::SetClipboardText(selected.c_str());
    }
}

void CodeEditor::paste() {
    if (readOnly) return;
    const char* clipboard = ImGui::GetClipboardText();
    if (clipboard && clipboard[0] != '\0') {
        insertText(clipboard);
    }
}

void CodeEditor::selectAll() {
    if (lines.empty()) return;
    selection.set(Position(0, 0),
                  Position((int)lines.size() - 1, (int)lines.back().length()));
    cursor = selection.end;
    ensureCursorVisible();
}

std::string CodeEditor::buildTextFromLines() const {
    std::string result;
    for (size_t i = 0; i < lines.size(); ++i) {
        result += lines[i];
        if (i + 1 < lines.size()) {
            result += '\n';
        }
    }
    return result;
}

std::string CodeEditor::getSelectedText() const {
    if (!selection.active || lines.empty()) return "";
    
    Position start = selection.start;
    Position end = selection.end;
    start.line = std::clamp(start.line, 0, (int)lines.size() - 1);
    end.line = std::clamp(end.line, 0, (int)lines.size() - 1);
    
    std::string result;
    for (int line = start.line; line <= end.line; ++line) {
        const std::string& currentLine = lines[line];
        int startCol = (line == start.line) ? start.column : 0;
        int endCol = (line == end.line) ? end.column : (int)currentLine.length();
        startCol = std::clamp(startCol, 0, (int)currentLine.length());
        endCol = std::clamp(endCol, 0, (int)currentLine.length());
        if (endCol > startCol) {
            result += currentLine.substr(startCol, endCol - startCol);
        }
        if (line != end.line) {
            result += '\n';
        }
    }
    
    return result;
}

void CodeEditor::renderHighlightedLine(ImDrawList* drawList, const std::string& line, ImVec2 textPos) {
    // Simple C/C++ style highlighter: keywords, types, strings, numbers, comments.
    static const char* keywords[] = {
        "if","else","switch","case","for","while","do","break","continue","return",
        "class","struct","public","private","protected","virtual","override","const",
        "constexpr","template","typename","using","namespace","new","delete","try","catch",
        "throw","auto","static","inline","friend","enum","operator","this","reinterpret_cast",
        "static_cast","dynamic_cast","const_cast","sizeof"
    };
    static const char* typeNames[] = {
        "int","long","short","float","double","char","bool","void","unsigned","signed",
        "size_t","std","string","vector","map"
    };
    
    auto isIdentStart = [](char c) { return std::isalpha(static_cast<unsigned char>(c)) || c == '_'; };
    auto isIdent = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
    
    ImU32 keywordColor = ImGui::GetColorU32(ImVec4(0.80f, 0.45f, 1.00f, 1.00f));
    ImU32 typeColor = ImGui::GetColorU32(ImVec4(0.55f, 0.80f, 1.00f, 1.00f));
    ImU32 stringColor = ImGui::GetColorU32(ImVec4(0.95f, 0.80f, 0.60f, 1.00f));
    ImU32 commentColor = ImGui::GetColorU32(ImVec4(0.45f, 0.65f, 0.45f, 1.00f));
    ImU32 numberColor = ImGui::GetColorU32(ImVec4(0.95f, 0.65f, 0.45f, 1.00f));
    ImU32 defaultColor = ImGui::GetColorU32(ImGuiCol_Text);
    
    float xOffset = 0.0f;
    size_t i = 0;
    while (i < line.size()) {
        // Comments
        if (i + 1 < line.size() && line[i] == '/' && line[i + 1] == '/') {
            std::string comment = line.substr(i);
            drawList->AddText(ImVec2(textPos.x + xOffset, textPos.y), commentColor, comment.c_str());
            break;
        }
        
        // Strings
        if (line[i] == '"') {
            size_t start = i;
            ++i;
            while (i < line.size()) {
                if (line[i] == '\\' && i + 1 < line.size()) {
                    i += 2;
                    continue;
                }
                if (line[i] == '"') {
                    ++i;
                    break;
                }
                ++i;
            }
            std::string token = line.substr(start, i - start);
            drawList->AddText(ImVec2(textPos.x + xOffset, textPos.y), stringColor, token.c_str());
            xOffset += ImGui::CalcTextSize(token.c_str()).x;
            continue;
        }
        
        // Numbers
        if (std::isdigit(static_cast<unsigned char>(line[i]))) {
            size_t start = i;
            while (i < line.size() && (std::isdigit(static_cast<unsigned char>(line[i])) || line[i]=='.')) {
                ++i;
            }
            std::string token = line.substr(start, i - start);
            drawList->AddText(ImVec2(textPos.x + xOffset, textPos.y), numberColor, token.c_str());
            xOffset += ImGui::CalcTextSize(token.c_str()).x;
            continue;
        }
        
        // Identifiers / keywords / types
        if (isIdentStart(line[i])) {
            size_t start = i;
            ++i;
            while (i < line.size() && isIdent(line[i])) ++i;
            std::string token = line.substr(start, i - start);
            
            auto isTokenIn = [&token](const char* const* list, size_t count) {
                for (size_t idx = 0; idx < count; ++idx) {
                    if (token == list[idx]) return true;
                }
                return false;
            };
            
            ImU32 color = defaultColor;
            if (isTokenIn(keywords, sizeof(keywords) / sizeof(keywords[0]))) {
                color = keywordColor;
            } else if (isTokenIn(typeNames, sizeof(typeNames) / sizeof(typeNames[0]))) {
                color = typeColor;
            }
            
            drawList->AddText(ImVec2(textPos.x + xOffset, textPos.y), color, token.c_str());
            xOffset += ImGui::CalcTextSize(token.c_str()).x;
            continue;
        }
        
        // Single character fallback
        char ch[2] = { line[i], 0 };
        drawList->AddText(ImVec2(textPos.x + xOffset, textPos.y), defaultColor, ch);
        xOffset += ImGui::CalcTextSize(ch).x;
        ++i;
    }
}

void CodeEditor::pushUndoState() {
    EditorState state;
    state.linesSnapshot = lines;
    state.cursorSnapshot = cursor;
    state.selectionSnapshot = selection;
    state.modifiedSnapshot = modified;
    
    undoStack.push_back(std::move(state));
    if (undoStack.size() > maxHistoryStates) {
        undoStack.erase(undoStack.begin());
    }
}

void CodeEditor::restoreState(const EditorState& state) {
    lines = state.linesSnapshot;
    cursor = state.cursorSnapshot;
    selection = state.selectionSnapshot;
    modified = state.modifiedSnapshot;
    maxLineWidth = -1;
    rebuildText();
}

bool CodeEditor::beginHistoryEntry() {
    if (historyLocked) return false;
    pushUndoState();
    historyLocked = true;
    redoStack.clear();
    return true;
}

void CodeEditor::endHistoryEntry(bool locked) {
    if (locked) {
        historyLocked = false;
    }
}

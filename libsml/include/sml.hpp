/**
 * @file sml.hpp
 * @brief Handles parsing and writing of sml streams
 */
#pragma once
#include <string>
#include <vector>
#include <map>
#include <istream>
#include <stack>
#include <algorithm>

namespace sml
{
    /**
     * @brief Represents a sml tag 
     * @remarks Contains child or content but not both
     */
    struct Node
    {
        std::string tagName;
        std::string content;
        std::vector<Node> children;
        std::map<std::string, std::string> attributes;
    };

    /**
     * @brief General error throw by the parser when it cannot continue.
     */
    class ParserError : public std::runtime_error
    {
    public:
        ParserError(const std::string& msg) : std::runtime_error("Parser error: " + msg)
        {
        }
    };

    /**
     * @brief Builder class which constructs a node tree from a stream of chars
     */
    class Parser
    {
    private:
        enum class State
        { 
            NULL_STATE, 
            NAME_STATE, 
            TAG_WHITESPACE, 
            TAG_ATTRIB_NAME,
            TAG_ATTRIB_EQUALS,
            TAG_ATTRIB_VALUE,
            
            TAG_CLOSE_NAME
        };

        enum class CharacterOperation
        {
            RECYCLE_CHAR,
            CONSUME_CHAR
        };

        struct StateTransition
        {
            CharacterOperation charOp;
            State newState;

            StateTransition(CharacterOperation op, State nState) : charOp(op), newState(nState)
            {
            }
        };

        std::stack<Node> m_nodeStack;

        std::string m_currentAttribName;
        std::string m_currentAttribValue;

        bool m_equalsSeen = false;
        bool m_terminalTag = false;

        StateTransition nullStateHandleChar(char c);
        StateTransition nameStateHandleChar(char c);
        StateTransition tagWhitespaceStateHandleChar(char c);
        StateTransition tagAttribNameHandleChar(char c);
        StateTransition tagAttribEqualsHandleChar(char c);
        StateTransition tagAttribValueHandleChar(char c);
        StateTransition tagCloseHandleChar(char c);

        State m_currentState = State::NULL_STATE;

    public:
        /**
         * @brief Resets the builder to an initial state to build another node tree
         */
        void reset();

        /**
         * @brief Gives the builder another character and handles it based on its current state
         * 
         * @param c character to handle
         */
        void handleChar(char c);
        
        /**
         * @brief Finializes the construction of the node tree and resets the builder
         * 
         * @return Node The built node tree 
         */
        Node finish();
    };

    std::istream& operator>>(std::istream& str, sml::Node& node);
    std::ostream& operator<<(std::ostream& str, sml::Node& node);

    /**
     * @brief Parses sml from a given iterator range 
     * 
     * @tparam IteratorType The iterator type to be used
     * @param begin The starting iterator
     * @param end The ending iterator
     * @return Node The node built from parsing the iterator range 
     */
    template<typename IteratorType>
    static Node parse(IteratorType begin, IteratorType end)
    {
        Parser p;
        std::for_each(begin, end, [&p](const auto& c){ p.handleChar(c); });
        return p.finish();
    }

    /**
     * @brief Parses sml from a given string 
     * 
     * @param str The string to be interpreted as sml
     * @return Node The node built from parsing the string
     */
    Node parse(const std::string& str);

    /**
     * @brief Parses sml from a given input stream 
     * 
     * @param str The input stream to be interpreted as sml
     * @return Node The node built from parsing the string
     */
    Node parse(std::istream& str);

    /**
     * @brief Writes out a sml node to a given output stream
     * 
     * @param node The node to be serialised
     * @param output The output stream to be used
     */
    void write(const Node& node, std::ostream& output);

    /**
     * @brief Writes out a sml node to a given output string
     * 
     * @param node The node to be serialised
     * @param output The string to be outputed
     */
    void write(const Node& node, std::string& output);
}
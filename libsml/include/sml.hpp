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
     * @brief Represents the location of a symbol within some source file
     */
    struct Location
    {
        std::size_t column, line;
    };

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

        std::size_t contentOffset; // location with the parents tags content
        Location location; // location from the source stream
    };

    /**
     * @brief General error throw by the parser when it cannot continue.
     */
    class ParserError : public std::runtime_error
    {
    public:
        Location location;

        ParserError(const std::string& msg, const Location& loc) : 
            std::runtime_error("Parser error: " + msg + " at " + std::to_string(loc.line) + ":"  + std::to_string(loc.column)), location(loc)
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
            START, // state before parsing a tag
            NAME, // tag name
            WHITESPACE, // seperators between attribs
            ATTRIB_NAME, // attrib=
            ATTRIB_EQUALS, // before equals
            ATTRIB_EQUALS_SEEN, // after equals
            ATTRIB_VALUE, // "value"
            CLOSE_NAME, // close tag
            SINGLETON // open tag closed with prefixed '/'
        };

        enum class CharOp
        {
            DEFER,
            CONSUME
        };

        struct StateChange
        {
            CharOp op;
            State nextState;
        };

        std::stack<Node> m_nodeStack;

        std::string m_currentAttribName;
        std::string m_currentAttribValue;

        bool m_equalsSeen = false;
        bool m_terminalTag = false;
        bool m_rootClosed = false;

        StateChange start(char c);
        StateChange name(char c);
        StateChange whitespace(char c);
        StateChange attrib_name(char c);
        StateChange attrib_equals(char c);
        StateChange attrib_equals_seen(char c);
        StateChange attrib_value(char c);
        StateChange close_name(char c);
        StateChange singleton(char c);

        State m_currentState = State::START;

        Location m_currentLocation = Location{ 1, 1 };

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
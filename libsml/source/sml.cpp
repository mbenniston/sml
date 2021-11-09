#include "../include/sml.hpp"
#include <iterator>
#include <sstream>

namespace sml
{
    //remove newlines and pre and post whitespace from 'raw'
    static std::pair<std::size_t, std::size_t> stripForContent(std::string &raw)
    {
        std::size_t numRemovedFromLeft = raw.size();

        raw.erase(raw.begin(), std::find_if(raw.begin(), raw.end(),
                                            [](char ch)
                                            { return !std::isspace(ch); }));
        numRemovedFromLeft = numRemovedFromLeft - raw.size();

        std::size_t numRemovedFromRight = raw.size();

        raw.erase(std::find_if(raw.rbegin(), raw.rend(),
                               [](char ch)
                               { return !std::isspace(ch); })
                      .base(),
                  raw.end());
        numRemovedFromRight = numRemovedFromRight - raw.size();

        return {numRemovedFromLeft, numRemovedFromRight};
    }

    static void stripNode(Node &node)
    {
        // strip content
        auto numRemoved = stripForContent(node.content);
        for (Node &child : node.children)
        {
            child.contentOffset -= numRemoved.first;

            if (child.contentOffset >= node.content.size())
            {
                child.contentOffset = node.content.size() - 1;
            }
        }
    }

    static constexpr bool isValidNameChar(char c)
    {
        return !std::isspace(c) && c != '<' && c != '>' && c != '=' && c != '/';
    }

    static constexpr bool isWhitespace(char c)
    {
        return std::isspace(c);
    }

    static constexpr char OPEN_TAG = '<';
    static constexpr char TAG_END = '>';
    static constexpr char CLOSE_TAG_PREFIX = '/';
    static constexpr char ATTRIB_EQUALS = '=';
    static constexpr char ATTRIB_VALUE_WRAP = '"';

    Parser::StateChange Parser::start(char c)
    {
        if (c == OPEN_TAG)
        {
            if (m_rootClosed)
            {
                throw ParserError("opening new tag when the root tag has already been closed",
                                  m_currentLocation);
            }

            // create new tag
            // hook the tag into its parents content if it exists
            std::size_t contentOffset = 0;
            if (!m_nodeStack.empty())
            {
                contentOffset = m_nodeStack.top().content.size();
            }

            m_nodeStack.emplace();
            m_nodeStack.top().location = m_currentLocation;
            m_nodeStack.top().contentOffset = contentOffset;

            return StateChange{CharOp::CONSUME, State::NAME};
        }

        // if there is a tag on the stack add to its content
        if (m_rootClosed && !std::isspace(c))
        {
            throw ParserError("declaring content when the root tag has already been closed",
                              m_currentLocation);
        }

        if (!m_nodeStack.empty())
        {
            m_nodeStack.top().content.push_back(c);
        }
        else
        {
            throw ParserError(std::string("expected root tag, got unexpected character: \"") + c + "\"",
                              m_currentLocation);
        }

        return StateChange{CharOp::CONSUME, State::START};
    }

    Parser::StateChange Parser::name(char c)
    {
        if (c == CLOSE_TAG_PREFIX)
        {
            if (m_nodeStack.top().tagName.empty())
            {
                return StateChange{CharOp::CONSUME, State::CLOSE_NAME};
            }
            else
            {
                // if the current name is not empty than the user is telling the parser to create a singleton
                return StateChange{CharOp::CONSUME, State::SINGLETON};
            }
        }

        if (isValidNameChar(c))
        {
            // build tag name
            m_nodeStack.top().tagName.push_back(c);

            return StateChange{CharOp::CONSUME, State::NAME};
        }

        if (c == TAG_END)
        {
            return StateChange{CharOp::DEFER, State::WHITESPACE};
        }

        if (isWhitespace(c))
        {
            return StateChange{CharOp::CONSUME, State::WHITESPACE};
        }

        throw ParserError(std::string("expected tag name got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    Parser::StateChange Parser::whitespace(char c)
    {
        if (isWhitespace(c))
        {
            return StateChange{CharOp::CONSUME, State::WHITESPACE};
        }

        if (isValidNameChar(c))
        {
            return StateChange{CharOp::DEFER, State::ATTRIB_NAME};
        }

        if (c == TAG_END)
        {
            // terminate the open tag
            return StateChange{CharOp::CONSUME, State::START};
        }

        if (c == CLOSE_TAG_PREFIX)
        {
            return StateChange{CharOp::CONSUME, State::SINGLETON};
        }

        throw ParserError(std::string("expected attrib name got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    Parser::StateChange Parser::attrib_name(char c)
    {
        if (isValidNameChar(c))
        {
            // build attrib name
            m_currentAttribName.push_back(c);

            return StateChange{CharOp::CONSUME, State::ATTRIB_NAME};
        }

        // if not a name expect whitespace or equals
        return StateChange{CharOp::DEFER, State::ATTRIB_EQUALS};
    }

    Parser::StateChange Parser::attrib_equals(char c)
    {
        if (isWhitespace(c))
        {
            return StateChange{CharOp::CONSUME, State::ATTRIB_EQUALS};
        }

        if (c == ATTRIB_EQUALS)
        {
            return StateChange{CharOp::CONSUME, State::ATTRIB_EQUALS_SEEN};
        }

        throw ParserError(std::string("expected \"=\" got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    Parser::StateChange Parser::attrib_equals_seen(char c)
    {
        if (isWhitespace(c))
        {
            return StateChange{CharOp::CONSUME, State::ATTRIB_EQUALS_SEEN};
        }

        if (c == ATTRIB_VALUE_WRAP)
        {
            return StateChange{CharOp::CONSUME, State::ATTRIB_VALUE};
        }

        throw ParserError(std::string("expected '\"' got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    Parser::StateChange Parser::attrib_value(char c)
    {
        if (c == ATTRIB_VALUE_WRAP)
        {
            // add value and key to attrib map
            m_nodeStack.top().attributes[m_currentAttribName] = m_currentAttribValue;

            m_currentAttribName.clear();
            m_currentAttribValue.clear();

            return StateChange{CharOp::CONSUME, State::WHITESPACE};
        }

        // build value
        m_currentAttribValue.push_back(c);

        return StateChange{CharOp::CONSUME, State::ATTRIB_VALUE};
    }

    Parser::StateChange Parser::close_name(char c)
    {
        if (isValidNameChar(c))
        {
            // build close tag name
            m_nodeStack.top().tagName.push_back(c);

            return StateChange{CharOp::CONSUME, State::CLOSE_NAME};
        }

        if (c == TAG_END)
        {
            // check that the close tag actually terminates a currently open tag
            Node closeTag = std::move(m_nodeStack.top());
            m_nodeStack.pop();

            if (closeTag.tagName != m_nodeStack.top().tagName)
            {
                throw ParserError("expected close tag with tag name: \"" + m_nodeStack.top().tagName + "\" got: \"" + closeTag.tagName + "\"",
                                  m_currentLocation);
            }

            if (!m_nodeStack.empty())
            {
                stripNode(m_nodeStack.top());
            }

            // if the close tag matched the node at the top of the stack
            if (m_nodeStack.size() > 1)
            {
                Node tagToClose = std::move(m_nodeStack.top());
                m_nodeStack.pop();

                m_nodeStack.top().children.emplace_back(std::move(tagToClose));
            }
            else
            {
                m_rootClosed = true;
            }

            return StateChange{CharOp::CONSUME, State::START};
        }

        throw ParserError(std::string("expected '>' got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    Parser::StateChange Parser::singleton(char c)
    {
        if (c == TAG_END)
        {
            Node singletonTag = std::move(m_nodeStack.top());
            m_nodeStack.pop();
            m_nodeStack.top().children.emplace_back(std::move(singletonTag));

            return StateChange{CharOp::CONSUME, State::START};
        }

        throw ParserError(std::string("expected '>' after '/' to close singleton tag, got unexpected character: \"") + c + "\"",
                          m_currentLocation);
    }

    void Parser::handleChar(char c)
    {
        bool characterConsumed = false;
        while (!characterConsumed)
        {
            // invoke the current state
            Parser::StateChange handleResult{CharOp::CONSUME, State::START};
            switch (m_currentState)
            {
            case State::START:
                handleResult = start(c);
                break;
            case State::NAME:
                handleResult = name(c);
                break;
            case State::WHITESPACE:
                handleResult = whitespace(c);
                break;
            case State::ATTRIB_NAME:
                handleResult = attrib_name(c);
                break;
            case State::ATTRIB_EQUALS:
                handleResult = attrib_equals(c);
                break;
            case State::ATTRIB_EQUALS_SEEN:
                handleResult = attrib_equals_seen(c);
                break;
            case State::ATTRIB_VALUE:
                handleResult = attrib_value(c);
                break;
            case State::CLOSE_NAME:
                handleResult = close_name(c);
                break;
            case State::SINGLETON:
                handleResult = singleton(c);
                break;
            default:
                throw std::logic_error("parser in malformed state");
            }

            characterConsumed = false;

            switch (handleResult.op)
            {
            case CharOp::CONSUME:
                characterConsumed = true;
            case CharOp::DEFER:
                break;
            default:
                throw std::logic_error("unkown char operation");
            }

            m_currentState = handleResult.nextState;
        }

        // handle location
        if (c == '\n')
        {
            m_currentLocation.column = 1;
            m_currentLocation.line++;
        }
        else
        {
            m_currentLocation.column++;
        }
    }

    void Parser::reset()
    {
        m_currentState = State::START;

        while (!m_nodeStack.empty())
        {
            m_nodeStack.pop();
        }

        m_currentAttribName.clear();
        m_currentAttribValue.clear();

        m_equalsSeen = false;
        m_terminalTag = false;
        m_rootClosed = false;

        m_currentLocation.column = 1;
        m_currentLocation.line = 1;
    }

    Node Parser::finish()
    {
        if (m_currentState != State::START)
        {
            throw ParserError("unexpected eof", m_currentLocation);
        }

        if (m_nodeStack.empty())
        {
            throw ParserError("no root node found!", m_currentLocation);
        }

        if (m_nodeStack.size() > 1 || !m_rootClosed)
        {
            throw ParserError("unclosed tag: \"" + m_nodeStack.top().tagName + "\"", m_nodeStack.top().location);
        }

        Node root = std::move(m_nodeStack.top());
        m_nodeStack.pop();

        reset();

        return root;
    }

    std::istream &operator>>(std::istream &str, sml::Node &node)
    {
        sml::Parser p;

        char character;
        while (str.get(character))
        {
            p.handleChar(character);
        }

        node = p.finish();

        return str;
    }

    std::ostream &operator<<(std::ostream &str, const sml::Node &node)
    {
        write(node, str);
        return str;
    }

    Node parse(const std::string &str)
    {
        return parse(str.begin(), str.end());
    }

    Node parse(std::istream &str)
    {
        Node node;
        str >> node;
        return node;
    }

    void write(const Node &node, std::ostream &output)
    {
        struct Tag
        {
            const Node &node;
            bool expanded;

            Tag(const Node &n, bool exp = false) : node(n), expanded(exp)
            {
            }
        };

        std::stack<Tag> expandStack;
        std::size_t depth = 0;

        expandStack.push(Tag{node});

        while (!expandStack.empty())
        {
            if (!expandStack.top().expanded)
            {
                Tag &tagToExpand = expandStack.top();

                // open print tag
                std::fill_n(std::ostream_iterator<char>(output), depth, '\t');

                // print attributes
                output << "<" << tagToExpand.node.tagName;

                for (const auto &keyValue : tagToExpand.node.attributes)
                {
                    output << " " << keyValue.first << "=\"" << keyValue.second << "\"";
                }

                // create short tag if it has no children
                if (tagToExpand.node.children.empty() && tagToExpand.node.content.empty())
                {
                    expandStack.pop();

                    output << "/>\n";
                }
                else
                {
                    // expand children
                    std::for_each(
                        tagToExpand.node.children.rbegin(),
                        tagToExpand.node.children.rend(),
                        [&expandStack](const auto &child)
                        { expandStack.push(Tag{child}); });

                    depth++;
                    tagToExpand.expanded = true;

                    output << ">";

                    if (tagToExpand.node.content.empty())
                    {
                        output << "\n";
                    }

                    // print content
                    output << tagToExpand.node.content;
                }
            }
            else
            {
                // unwind expanded tags
                while (!expandStack.empty() && expandStack.top().expanded)
                {
                    depth--;

                    if (!expandStack.top().node.children.empty())
                    {
                        std::fill_n(std::ostream_iterator<char>(output), depth, '\t');
                    }

                    output << "</" << expandStack.top().node.tagName << ">" << std::endl;
                    expandStack.pop();
                }
            }
        }
    }

    void write(const Node &node, std::string &output)
    {
        std::ostringstream ss;
        write(node, ss);
        output = ss.str();
    }
}
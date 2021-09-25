#include "../include/sml.hpp"
#include <iostream>
#include <iterator>
#include <sstream>

namespace sml
{
    // remove newlines and pre and post whitespace from 'raw'
    static void stripForContent(std::string& raw)
    {
        raw.erase(raw.begin(), std::find_if(raw.begin(), raw.end(), 
            [](char ch) { return !std::isspace(ch); }));

        raw.erase(std::find_if(raw.rbegin(), raw.rend(), 
            [](char ch) { return !std::isspace(ch);}).base(), raw.end());

        raw.erase(std::remove(raw.begin(), raw.end(), '\n'),
            raw.end());

        raw.erase(std::remove(raw.begin(), raw.end(), '\t'),
            raw.end());
    }

    static constexpr bool isValidNameCharacter(char c)
    {
        return !std::isspace(c) && c != '<' && c != '>' && c != '=' && c != '/';
    }

    Parser::StateTransition Parser::nullStateHandleChar(char c)
    {
        if(c == '<')
        {
            m_nodeStack.emplace();
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::NAME_STATE);
        }

        if(!m_nodeStack.empty())
        {
            Node& topNode = m_nodeStack.top();
            if(topNode.children.empty())
            {
                topNode.content.push_back(c);
            }
            else if(!topNode.content.empty())
            {
                topNode.content.clear();
            }
        }

        return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::NULL_STATE);
    }

    Parser::StateTransition Parser::nameStateHandleChar(char c)
    {
        if(m_nodeStack.top().tagName.empty() && c == '/')
        {
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_CLOSE_NAME);
        }

        if(isValidNameCharacter(c))
        {
            m_nodeStack.top().tagName.push_back(c);

            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::NAME_STATE);
        }
        
        return StateTransition(CharacterOperation::RECYCLE_CHAR, Parser::State::TAG_WHITESPACE);
    }

    Parser::StateTransition Parser::tagWhitespaceStateHandleChar(char c)
    {
        if(m_terminalTag && c != '>')
        {
            throw ParserError("terminal tag symbol not follwed by then end of the tag");
        }

        if(std::isspace(c))
        {
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_WHITESPACE);
        }

        if(isValidNameCharacter(c))
        {
            return StateTransition(CharacterOperation::RECYCLE_CHAR, Parser::State::TAG_ATTRIB_NAME);
        }

        // tag end
        if(c == '>')
        {
            if(m_terminalTag)
            {
                if(m_nodeStack.size() > 1)
                {
                    Node tag = std::move(m_nodeStack.top());

                    if(!tag.children.empty() && !tag.content.empty())
                    {
                        tag.content.clear();
                    }
                    else
                    {
                        stripForContent(tag.content);
                    }

                    m_nodeStack.pop();
                    m_nodeStack.top().children.push_back(std::move(tag));
                }

                m_terminalTag = false;
            }

            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::NULL_STATE);
        }

        if(c == '/')
        {
            m_terminalTag = true;
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_WHITESPACE);
        }

        throw ParserError("unexpected character");
    }

    Parser::StateTransition Parser::tagAttribNameHandleChar(char c)
    {
        if(isValidNameCharacter(c))
        {
            m_currentAttribName.push_back(c);
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_ATTRIB_NAME);
        }

        if(std::isspace(c) || c == '=')
        {
            return StateTransition(CharacterOperation::RECYCLE_CHAR, Parser::State::TAG_ATTRIB_EQUALS);
        }

        throw ParserError("unexpected characeter");
    }

    Parser::StateTransition Parser::tagAttribEqualsHandleChar(char c)
    {
        if(std::isspace(c))
        {
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_ATTRIB_EQUALS);
        }

        if(c == '=' && !m_equalsSeen)
        {
            m_equalsSeen = true;
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_ATTRIB_EQUALS);
        }

        if(c == '"' && m_equalsSeen)
        {
            m_equalsSeen = false;
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_ATTRIB_VALUE);
        }
        else if(c == '"')
        {
            throw ParserError("equals not seen");
        }

        if(m_equalsSeen)
        {
            throw ParserError("multiple equals after attrib");
        }
        
        throw ParserError("unexpected characeter");
    }

    Parser::StateTransition Parser::tagAttribValueHandleChar(char c)
    {
        if(c == '"')
        {   
            m_nodeStack.top().attributes[m_currentAttribName] = m_currentAttribValue;
            m_currentAttribName.clear();
            m_currentAttribValue.clear();
            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_WHITESPACE);
        }

        m_currentAttribValue.push_back(c);

        return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_ATTRIB_VALUE);
    }

    Parser::StateTransition Parser::tagCloseHandleChar(char c)
    {
        if(isValidNameCharacter(c))
        {
            m_nodeStack.top().tagName.push_back(c);

            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::TAG_CLOSE_NAME);
        }

        if(c == '>')
        {
            Node closeTag = std::move(m_nodeStack.top());
            m_nodeStack.pop();

            if(m_nodeStack.top().tagName != closeTag.tagName)
            {
                std::cout << closeTag.tagName << std::endl;

                while(!m_nodeStack.empty())
                {
                    std::cout << m_nodeStack.top().tagName << std::endl;
                    m_nodeStack.pop();
                }

                throw ParserError("unclosed tag");
            }

            if(m_nodeStack.size() > 1)
            {
                Node tag = std::move(m_nodeStack.top());
                
                if(!tag.children.empty() && !tag.content.empty())
                {
                    tag.content.clear();
                }
                else
                {
                    stripForContent(tag.content);
                }
            
                m_nodeStack.pop();
                m_nodeStack.top().children.push_back(std::move(tag));
            }

            return StateTransition(CharacterOperation::CONSUME_CHAR, Parser::State::NULL_STATE);
        }

        throw ParserError("unexpected characeter");
    }

    void Parser::handleChar(char c)
    {
        bool characterConsumed = false;
        while(!characterConsumed)
        {
            // invoke the current state
            Parser::StateTransition handleResult{ CharacterOperation::CONSUME_CHAR, State::NULL_STATE };
            switch (m_currentState)
            {
            case State::NULL_STATE:
                handleResult = nullStateHandleChar(c);
                break;
            case State::NAME_STATE:
                handleResult = nameStateHandleChar(c);
                break;
            case State::TAG_WHITESPACE:
                handleResult = tagWhitespaceStateHandleChar(c);
                break;
            case State::TAG_ATTRIB_NAME:
                handleResult = tagAttribNameHandleChar(c);
                break;
            case State::TAG_ATTRIB_EQUALS:
                handleResult = tagAttribEqualsHandleChar(c);
                break;
            case State::TAG_ATTRIB_VALUE:
                handleResult = tagAttribValueHandleChar(c);
                break;
            case State::TAG_CLOSE_NAME:
                handleResult = tagCloseHandleChar(c);
                break;
            default:
                throw std::logic_error("unknown state");
                break;
            }

            characterConsumed = (handleResult.charOp == CharacterOperation::CONSUME_CHAR);
            m_currentState = handleResult.newState;
        }
    }

    void Parser::reset()
    {
        m_currentState = State::NULL_STATE;

        while(!m_nodeStack.empty())
        {
            m_nodeStack.pop();
        }

        m_currentAttribName.clear();
        m_currentAttribValue.clear();

        m_equalsSeen = false;
        m_terminalTag = false;
    }

    Node Parser::finish()
    {
        if(m_currentState != State::NULL_STATE)
        {
            throw ParserError("unexpected eof");
        }

        if(m_nodeStack.empty())
        {
            throw ParserError("no root node found!");
        }

        Node root = std::move(m_nodeStack.top());
        m_nodeStack.pop();

        reset();

        return root;
    }

    std::istream& operator>>(std::istream& str, sml::Node& node)
    {
        sml::Parser p;
        
        char character;
        while(str.get(character))
        {
            p.handleChar(character);
        }

        node = p.finish();

        return str;
    }

    std::ostream& operator<<(std::ostream& str, sml::Node& node)
    {
        write(node, str);
        return str;
    }

    Node parse(const std::string& str)
    {
        return parse(str.begin(), str.end());
    }

    Node parse(std::istream& str)
    {
        Node node;
        str >> node;
        return node;
    }

    void write(const Node& node, std::ostream& output)
    {
        struct Tag
        {
            const Node* node;
            bool expanded;

            Tag(const Node* n, bool exp = false) : node(n), expanded(exp)
            {
            }
        };

        std::stack<Tag> expandStack; 
        std::size_t depth = 0;

        expandStack.push(Tag{ &node });

        while(!expandStack.empty())
        {
            if(!expandStack.top().expanded)
            {
                Tag& tagToExpand = expandStack.top();

                // open print tag
                std::fill_n(std::ostream_iterator<char>(output), depth, '\t');

                // print attributes
                output << "<" << tagToExpand.node->tagName;

                for(const auto& keyValue : tagToExpand.node->attributes)
                {
                    output << " " << keyValue.first << "=\"" << keyValue.second << "\"";
                }

                // create short tag if it has no children
                if(tagToExpand.node->children.empty() && tagToExpand.node->content.empty())
                {
                    expandStack.pop();
                    
                    output << "/>\n";
                }
                else
                {
                    // expand children
                    std::for_each(
                        tagToExpand.node->children.rbegin(),
                        tagToExpand.node->children.rend(),
                        [&expandStack](const auto& child){ expandStack.push(Tag{ &child }); });
                    
                    depth++;
                    tagToExpand.expanded = true;

                    output << ">";

                    if(tagToExpand.node->content.empty())
                    {
                        output << "\n";
                    }

                    // print content
                    output << tagToExpand.node->content;
                }
            }
            else
            {
                // unwind expanded tags
                while(!expandStack.empty() && expandStack.top().expanded)
                {
                    depth--;

                    if(!expandStack.top().node->children.empty())
                    {
                        std::fill_n(std::ostream_iterator<char>(output), depth, '\t');
                    }

                    output << "</" << expandStack.top().node->tagName << ">" << std::endl;
                    expandStack.pop();
                }
            }
        }
    }

    void write(const Node& node, std::string& output)
    {
        std::ostringstream ss;
        write(node, ss);
        output = ss.str();
    }
}
/**
 *  @file CommandLineParser.cpp
 *  @brief Include file defining the CommandLineParser class.
 *
 *  This software is licensed under the GNU Lesser General Public License version 3.
 *  Please refer to gpl.txt and lgpl.txt for more information.
 *
 *  @author Draconic Entertainment
 *  @version 0.0.0.19
 *  @date 3/19/2014
 *  @copyright (c) 2014 Draconic Entertainment
 */

#include <support/CommandLineParser.hpp>

namespace Kiaro
{
    namespace Support
    {
        CommandLineParser::CommandLineParser(Kiaro::Common::S32 argc, Kiaro::Common::C8 **argv) : mCurrentLongestFlagLength(0)
        {
            mArgv = argv;

            static const boost::regex flagRegex("-.+", boost::regex::extended);

           	size_t currentFlagHash = 0;
			const Kiaro::Common::C8 *currentFlagName = NULL;

            for (size_t iteration = 1; iteration < argc; iteration++)
				if (boost::regex_match(argv[iteration], flagRegex))
				{
					currentFlagName = argv[iteration];
					currentFlagHash = Kiaro::Common::string_hash(currentFlagName);

					mFlags.insert(std::make_pair(currentFlagHash, std::vector<std::string>()));
				}
				else if (currentFlagName != NULL)
                    mFlags[currentFlagHash].push_back(argv[iteration]);

            FlagEntry *helpEntry = new FlagEntry;
            helpEntry->name = "-h";
            helpEntry->description = "Displays this help text.";
            helpEntry->responder = new FlagResponder::MemberDelegateType<CommandLineParser>(this, &CommandLineParser::displayHelp);

            setFlagResponder(helpEntry);
        }

        CommandLineParser::~CommandLineParser(void) { }

        bool CommandLineParser::hasFlag(const Kiaro::Common::C8 *flag)
        {
			size_t flagHash = Kiaro::Common::string_hash(flag);

			try
			{
				// NOTE (Robert MacGregor#1): Testing the existence of our flag by purposely raising an exception
				mFlags.at(flagHash);
				return true;
			}
			catch (const std::out_of_range &e)
			{
				return false;
			}

			return false;
        }

        std::string CommandLineParser::getFlagArgument(const Kiaro::Common::C8 *targetFlag, Kiaro::Common::U8 argumentIdentifier)
        {
            if (!hasFlag(targetFlag))
                return "";

            size_t flagHash = Kiaro::Common::string_hash(targetFlag);

            std::vector<std::string> &requestedFlagVector = mFlags[flagHash];
            if (argumentIdentifier >= requestedFlagVector.size())
                return "";

            return requestedFlagVector[argumentIdentifier];
        }

        size_t CommandLineParser::getFlagArgumentCount(const Kiaro::Common::C8 *targetFlag)
        {
            if (!hasFlag(targetFlag))
                return 0;

            size_t flagHash = Kiaro::Common::string_hash(targetFlag);
			return mFlags[flagHash].size();
        }

        void CommandLineParser::setFlagResponder(FlagEntry *entry)
        {
            size_t flagHash = Kiaro::Common::string_hash(entry->name);
            mFlagResponders[flagHash] = entry;
            mFlagEntries.push_back(entry);

            if (entry->name.length() > mCurrentLongestFlagLength)
                mCurrentLongestFlagLength = entry->name.length() + 5;
        }

        void CommandLineParser::invokeFlagResponders(void)
        {
            bool executedFlagHandler = false;

            for (boost::unordered_map<size_t, std::vector<std::string>>::iterator it = mFlags.begin(); it != mFlags.end(); it++)
                try
                {
                    size_t currentFlagHash = (*it).first;

                    FlagResponder *currentResponder = mFlagResponders.at(currentFlagHash)->responder;

                    // Don't try to execute the responder if there isn't one
                    if (!currentResponder)
                        continue;

                    currentResponder->invoke(this, mArgv, mFlags[currentFlagHash], executedFlagHandler);
                    executedFlagHandler = true;
                }
                catch (std::out_of_range &e) { }

            if (!executedFlagHandler)
            {
                size_t helpHash = Kiaro::Common::string_hash("-h");
                std::cout << "No command line arguments provided. " << std::endl << std::endl;
                displayHelp(this, mArgv, mFlags[helpHash], false);
            }
        }

        void CommandLineParser::displayHelp(CommandLineParser *parser, Kiaro::Common::C8 *argv[], const std::vector<std::string> &arguments, bool otherFlags)
        {
            std::cout << "You may run " << argv[0] << " with:" << std::endl;

            for (std::vector<FlagEntry *>::iterator it = mFlagEntries.begin(); it != mFlagEntries.end(); it++)
            {
                FlagEntry *currentEntry = (*it);

                // Build the seperator for this line
                std::string blankSeperator;
                for (size_t iteration = 0; iteration < mCurrentLongestFlagLength - currentEntry->name.length(); iteration++)
                    blankSeperator += " ";

                std::cout << "  " << currentEntry->name << blankSeperator << currentEntry->description << std::endl;
            }
        }
    } // End Namespace Support
} // End Namespace Kiaro

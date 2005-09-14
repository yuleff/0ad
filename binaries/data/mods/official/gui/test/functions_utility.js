/*
	DESCRIPTION	: Generic utility functions.
	NOTES		: 
*/

// ====================================================================

function getRandom(randomMin, randomMax)
{
	// Returns a random whole number in a min..max range.
	// NOTE: There should probably be an engine function for this,
	// since we'd need to keep track of random seeds for replays.

	randomNum = randomMin + (randomMax-randomMin)*Math.random();  // num is random, from A to B 
	return Math.round(randomNum);
}

// ====================================================================

function parseDelimiterString (parseString, Delimiter) 
{ 
        // Seeks through the delimiters in a string and populates the elements of an array with fields found between them. 

        // Declare local variables. 
        parseLoop = 0; 
        parseElement = 0; 
        seekDelimiter = 0; 
        parseArray = new Array(); 

        // While we're still within the bounds of the string, 
        while (parseLoop <= parseString.length) 
        { 
                // Seek until we find a delimiter. 
                seekDelimiter = parseLoop; 
                while (parseString[seekDelimiter] != Delimiter && seekDelimiter <= parseString.length) 
                        seekDelimiter++; 

                // If we found a delimiter within the string, 
                if (seekDelimiter != parseString.length) 
                { 
                        // Store sub-string between start point and delimiter in array element. 
                        parseArray[parseElement] = parseString.substring(parseLoop, seekDelimiter); 
                        parseElement++; 
                } 

                // Move to after delimiter position for next seek. 
                parseLoop = seekDelimiter+1; 
        } 

        // Store length of array. 
        parseArray.length = parseElement; 

        return parseArray; 
}

// ====================================================================

function addArrayElement(Array) 
{ 
        // Adds an element to an array, updates its given index, and returns the index of the element. 

        Array[Array.last] = new Object(); 
        Array.last++; 

        return (Array.last - 1); 
} 

// ====================================================================

function toTitleCase (string)
{
	// Returns the title-case version of a given string.

	string = string.substring(0,1).toUpperCase() + string.substring(1, string.length).toLowerCase();
	return (string);
}

// ====================================================================
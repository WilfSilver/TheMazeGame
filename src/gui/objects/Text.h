#pragma once

#include <GLM.h>
#include <string>

struct Text
{
	float x, y;

	std::string m_Text;
	float       m_Scale;
	glm::vec4   m_Colour;

	bool isCentered, isOverlay;

	Text(std::string text);
	Text(std::string text, float x, float y, float scale, glm::vec4 colour, bool isCentered, bool isOverlay);
	~Text();

	void render(float xOffset, float yOffset);
};
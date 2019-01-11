class Color
{
private:
  float _red;
  float _green;
  float _blue;
  float _alpha;

public:
  Color(float red, float green, float blue, float alpha = 1.f) : _red(red), _green(green), _blue(blue), _alpha(alpha)
  {}

    /*! @brief Gets the red value of the color.
     *
     * This function returns the red value (between 0 and 1) of the color.
     *
     * @return The red value (between 0 and 1).
     */
    float red() const
    {
      return _red;
    }

    /*! @brief Gets the green value of the color.
     *
     * This function returns the green value (between 0 and 1) of the color.
     *
     * @return The green value (between 0 and 1).
     */
    float green() const
    {
      return _green;
    }

    /*! @brief Gets the blue value of the color.
     *
     * This function returns the red value (between 0 and 1) of the color.
     *
     * @return The blue value (between 0 and 1).
     */
    float blue() const
    {
      return _blue;
    }

    /*! @brief Gets the alpha value of the color.
     *
     * This function returns the alpha value (between 0 and 1) of the color.
     *
     * @return The alpha value (between 0 and 1).
     */
    float alpha() const
    {
      return _alpha;
    }
};

namespace color
{
  /*!
   * Makes a new Color instance from the given hexadecimal color value.
   * @param hex_color The hexadecimal color value.
   * @param has_alpha True if the hexadecimal color value includes the alpha channel, else false.
   * @return A new Color instance.
   */
  //extern Color from_hex(uint64_t hex_color, bool has_alpha = true);

  /*!
   * Makes a new Color instance from the given hexadecimal color value string.
   * @param hex_color The hexadecimal color value as a string.
   * @return A new Color instance.
   */
  //extern Color from_hex(const String &hex_color);

  /*!
   * Makes a new Color instance from a RGB value.
   * @param red Red channel.
   * @param green Green channel.
   * @param blue Blue channel.
   * @param alpha Alpha channel.
   * @return A new Color instance.
   */
  extern Color from_int_rgba(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255)
  {
    return {(red / 255.f), (green / 255.f), (blue / 255.f), (alpha / 255.f)};
  }
}

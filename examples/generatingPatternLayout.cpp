#include "Vernier.hpp"

using namespace vernier;
using namespace std;

/** This example shows how to generate pattern layouts from a JSON file.
 */
int main() {
    
    string filename = "megarenaPattern";
    
    // Loading the layout
    PatternLayout* layout = Layout::loadFromJSON(filename + ".json");
    cout << "Pattern layout: " << layout->toString() << endl;

    // Writing the layout in a PNG file
    layout->saveToPNG(filename + ".png");
    cout << filename + ".png generation completed." << endl;
    
    // Writing the layout in a SVG file
    layout->saveToSVG(filename + ".svg");
    cout << filename + ".svg generation completed." << endl;
    
    delete layout;
}

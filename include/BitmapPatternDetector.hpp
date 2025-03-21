/* 
 * This file is part of the VERNIER Library.
 *
 * Copyright (c) 2018-2025 CNRS, ENSMM, UMLP.
 */

#ifndef BITMAPPATTERNDETECTOR_HPP
#define BITMAPPATTERNDETECTOR_HPP

#include "PeriodicPatternDetector.hpp"

namespace vernier {

    /** \brief Class to estimate the absolute pose of periodic encoded patterns with subpixel resolutions.
     */
    class BitmapPatternDetector : public PeriodicPatternDetector {
    protected:

        cv::Mat thumbnail;
        std::vector<cv::Mat> bitmap;

        void readJSON(rapidjson::Value& document) override;

        void computeAbsolutePose(const Eigen::ArrayXXd & pattern);
        
        void computeThumbnail(const Eigen::ArrayXXd & array, double deltaPhase);

    public:
        
        int THUMBNAIL_ZOOM = 5;

        /** Constructs an empty detector for bitmap patterns */
        BitmapPatternDetector();
        
        /** Constructs a detector for bitmap patterns with a specific image
         *
         *	\param physicalPeriod: physical period of the pattern used to build it
         *	\param filename: name if the image file
         */
        BitmapPatternDetector(double physicalPeriod, const std::string filename);
        
        void computeArray(const Eigen::ArrayXXd & array) override;
        
        void showControlImages() override;
        
        /** Returns the computed thumbnail of the image 
         */ 
        cv::Mat getThumbnail();

        int getInt(const std::string & attribute) override;
        
        void* getObject(const std::string & attribute) override;

    };
}

#endif
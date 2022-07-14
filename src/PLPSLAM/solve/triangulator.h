/**
 * This file is part of Structure PLP-SLAM, originally from OpenVSLAM.
 *
 * Copyright 2022 DFKI (German Research Center for Artificial Intelligence)
 * Modified by Fangwen Shu <Fangwen.Shu@dfki.de>
 *
 * If you use this code, please cite the respective publications as
 * listed on the github repository.
 *
 * Structure PLP-SLAM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Structure PLP-SLAM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Structure PLP-SLAM. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLPSLAM_SOLVE_TRIANGULATOR_H
#define PLPSLAM_SOLVE_TRIANGULATOR_H

#include "PLPSLAM/type.h"

#include <Eigen/SVD>
#include <opencv2/core.hpp>

namespace PLPSLAM
{
    namespace solve
    {

        class triangulator
        {
        public:
            /**
             * Triangulate using two points and two perspective projection matrices
             * @param pt_1
             * @param pt_2
             * @param P_1
             * @param P_2
             * @return triangulated point in the world reference
             */
            static inline Vec3_t triangulate(const cv::Point2d &pt_1, const cv::Point2d &pt_2, const Mat34_t &P_1, const Mat34_t &P_2);

            /**
             * Triangulate using two bearings and relative rotation & translation
             * @param bearing_1
             * @param bearing_2
             * @param rot_21
             * @param trans_21
             * @return triangulated point in the camera 1 coordinates
             */
            static inline Vec3_t triangulate(const Vec3_t &bearing_1, const Vec3_t &bearing_2, const Mat33_t &rot_21, const Vec3_t &trans_21);

            /**
             * Triangulate using two bearings and absolute camera poses
             * @param bearing_1
             * @param bearing_2
             * @param cam_pose_1
             * @param cam_pose_2
             * @return
             */
            static inline Vec3_t triangulate(const Vec3_t &bearing_1, const Vec3_t &bearing_2, const Mat44_t &cam_pose_1, const Mat44_t &cam_pose_2);
        };

        Vec3_t triangulator::triangulate(const cv::Point2d &pt_1, const cv::Point2d &pt_2, const Mat34_t &P_1, const Mat34_t &P_2)
        {
            Mat44_t A;

            A.row(0) = pt_1.x * P_1.row(2) - P_1.row(0);
            A.row(1) = pt_1.y * P_1.row(2) - P_1.row(1);
            A.row(2) = pt_2.x * P_2.row(2) - P_2.row(0);
            A.row(3) = pt_2.y * P_2.row(2) - P_2.row(1);

            const Eigen::JacobiSVD<Mat44_t> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);

            const Vec4_t v = svd.matrixV().col(3);
            return v.block<3, 1>(0, 0) / v(3);
        }

        Vec3_t triangulator::triangulate(const Vec3_t &bearing_1, const Vec3_t &bearing_2, const Mat33_t &rot_21, const Vec3_t &trans_21)
        {
            const Vec3_t trans_12 = -rot_21.transpose() * trans_21;
            const Vec3_t bearing_2_in_1 = rot_21.transpose() * bearing_2;

            Mat22_t A;
            A(0, 0) = bearing_1.dot(bearing_1);
            A(1, 0) = bearing_1.dot(bearing_2_in_1);
            A(0, 1) = -A(1, 0);
            A(1, 1) = -bearing_2_in_1.dot(bearing_2_in_1);

            const Vec2_t b{bearing_1.dot(trans_12), bearing_2_in_1.dot(trans_12)};

            const Vec2_t lambda = A.inverse() * b;
            const Vec3_t pt_1 = lambda(0) * bearing_1;
            const Vec3_t pt_2 = lambda(1) * bearing_2_in_1 + trans_12;
            return (pt_1 + pt_2) / 2.0;
        }

        Vec3_t triangulator::triangulate(const Vec3_t &bearing_1, const Vec3_t &bearing_2, const Mat44_t &cam_pose_1, const Mat44_t &cam_pose_2)
        {
            Mat44_t A;
            A.row(0) = bearing_1(0) * cam_pose_1.row(2) - bearing_1(2) * cam_pose_1.row(0);
            A.row(1) = bearing_1(1) * cam_pose_1.row(2) - bearing_1(2) * cam_pose_1.row(1);
            A.row(2) = bearing_2(0) * cam_pose_2.row(2) - bearing_2(2) * cam_pose_2.row(0);
            A.row(3) = bearing_2(1) * cam_pose_2.row(2) - bearing_2(2) * cam_pose_2.row(1);

            // Aを特異値分解する (A = U S Vt)
            // https://eigen.tuxfamily.org/dox/classEigen_1_1JacobiSVD.html
            Eigen::JacobiSVD<Mat44_t> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
            const Vec4_t singular_vector = svd.matrixV().block<4, 1>(0, 3);

            return singular_vector.block<3, 1>(0, 0) / singular_vector(3);
        }

    } // namespace solve
} // namespace PLPSLAM

#endif // PLPSLAM_SOLVE_TRIANGULATOR_H


#include <iostream>
#include <opencv2/opencv.hpp>
#include <opencv2/geometry/2d.hpp>
#include <vector>
#include <string>
#include <algorithm>

int main() {
    // Otwieramy kamerę nr 0
    cv::VideoCapture camera(0);

    // Sprawdzamy, czy kamera została poprawnie otwarta
    if (!camera.isOpened())
    {
        std::cerr << "Failed to open camera!" << std::endl;
        return 1;
    }

    // Ustawiamy rozdzielczość kamery na 1280x720
    camera.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    camera.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    // Wyświetlamy rzeczywistą rozdzielczość kamery
    std::cout << "Camera width: "
              << camera.get(cv::CAP_PROP_FRAME_WIDTH) << '\n';

    std::cout << "Camera height: "
              << camera.get(cv::CAP_PROP_FRAME_HEIGHT) << '\n';

    cv::Mat frame;

    // Domyślnie odbicie lustrzane jest wyłączone
    bool mirror = false;

    // Główna pętla programu
    while (true) {
        // Pobieramy nową klatkę z kamery
        if (!camera.read(frame) || frame.empty())
        {
            std::cerr << "Failed to read frame!" << std::endl;
            break;
        }

        // Przygotowujemy obraz do wyświetlenia
        cv::Mat displayFrame;

        if (mirror)
        {
            // Odbijamy obraz w poziomie
            cv::flip(frame, displayFrame, 1);
        }
        else
        {
            // Kopiujemy oryginalny obraz
            displayFrame = frame.clone();
        }

        // Konwertujemy obraz na skalę szarości
        cv::Mat gray;
        cv::cvtColor(displayFrame, gray, cv::COLOR_BGR2GRAY);

        // Konwertujemy obraz do przestrzeni kolorów HSV
        cv::Mat hsv;
        cv::cvtColor(displayFrame, hsv, cv::COLOR_BGR2HSV);

        // Tworzymy maskę kolorów skóry
        cv::Mat mask;

        cv::Scalar lower(0, 55, 100);
        cv::Scalar upper(25, 200, 255);

        cv::inRange(hsv, lower, upper, mask);

        // Wygładzamy obraz w skali szarości
        cv::Mat blurred;
        cv::GaussianBlur(
            gray,
            blurred,
            cv::Size(5, 5),
            0
        );

        // Tworzymy obraz binarny
        // Obecnie służy on tylko do wyświetlania
        cv::Mat binary;
        cv::threshold(
            blurred,
            binary,
            127,
            255,
            cv::THRESH_BINARY
        );

        // Wyszukujemy zewnętrzne kontury na masce HSV
        std::vector<std::vector<cv::Point>> contours;

        cv::findContours(
            mask,
            contours,
            cv::RETR_EXTERNAL,
            cv::CHAIN_APPROX_SIMPLE
        );

        // Przechowujemy indeksy wszystkich odpowiednich konturów
        std::vector<int> handContours;

        for (int i = 0; i < static_cast<int>(contours.size()); i++)
        {
            // Obliczamy pole powierzchni konturu
            double area = cv::contourArea(contours[i]);

            // Odrzucamy zbyt małe i zbyt duże kontury
            if (area > 300 && area < 20000)
            {
                handContours.push_back(i);
            }
        }

        // Analizujemy każdy znaleziony kontur
        for (int contourIndex : handContours) {
            const auto& handContour = contours[contourIndex];

            // Znajdujemy skrajne punkty konturu dłoni
            cv::Point topPoint = handContour[0];
            cv::Point bottomPoint = handContour[0];
            cv::Point leftPoint = handContour[0];
            cv::Point rightPoint = handContour[0];

            for (const cv::Point& point : handContour)
            {
                if (point.y < topPoint.y)
                    topPoint = point;

                if (point.y > bottomPoint.y)
                    bottomPoint = point;

                if (point.x < leftPoint.x)
                    leftPoint = point;

                if (point.x > rightPoint.x)
                    rightPoint = point;
            }

            // Rysujemy skrajne punkty
            cv::circle(displayFrame, topPoint, 8, cv::Scalar(255, 0, 0), -1);
            cv::circle(displayFrame, bottomPoint, 8, cv::Scalar(0, 255, 255), -1);
            cv::circle(displayFrame, leftPoint, 8, cv::Scalar(255, 0, 255), -1);
            cv::circle(displayFrame, rightPoint, 8, cv::Scalar(0, 255, 0), -1);

            // Wyznaczamy otoczkę wypukłą dłoni
            std::vector<cv::Point> hullPoints;
            cv::convexHull(handContour, hullPoints, false, true);

            // Rysujemy otoczkę i zaznaczamy jej wierzchołki
            if (hullPoints.size() >= 3)
            {
                std::vector<std::vector<cv::Point>> hullContours = {hullPoints};

                cv::polylines(
                    displayFrame,
                    hullContours,
                    true,
                    cv::Scalar(255, 255, 0),
                    2
                );
            }


            // Wyświetlamy liczbę wierzchołków otoczki
            cv::putText(
                displayFrame,
                "Hull points: " + std::to_string(hullPoints.size()),
                cv::Point(30, 40),
                cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                cv::Scalar(0, 255, 255),
                2
            );
            // Rysujemy kontur na zielono
            cv::drawContours(
                displayFrame,
                contours,
                contourIndex,
                cv::Scalar(0, 255, 0),
                2
            );

            // Wyznaczamy indeksy punktów otoczki wypukłej
            std::vector<int> hullIndices;

            cv::convexHull(
                handContour,
                hullIndices,
                false,
                false
            );

            // Wyszukujemy zagłębienia między konturem a otoczką
            std::vector<cv::Vec4i> defects;

            if (hullIndices.size() >= 3 &&
                handContour.size() >= 4)
            {
                try
                {
                    cv::convexityDefects(
                        handContour,
                        hullIndices,
                        defects
                    );
                }
                catch (const cv::Exception& e)
                {
                    // Pomijamy nieprawidłowy kontur w bieżącej klatce
                    defects.clear();
                }
            }


            // Szukamy potencjalnych koniuszków palców
            std::vector<cv::Point> fingerTips;

            for (const cv::Vec4i& defect : defects) {
                // Pobieramy indeksy punktów zagłębienia
                int startIndex = defect[0];
                int endIndex = defect[1];
                int farIndex = defect[2];

                // Obliczamy głębokość zagłębienia
                float depth = defect[3] / 256.0f;

                // Pomijamy płytkie zagłębienia
                if (depth < 15.0f)
                    continue;

                // Pobieramy współrzędne punktów
                cv::Point start = handContour[startIndex];
                cv::Point end = handContour[endIndex];
                cv::Point farPoint = handContour[farIndex];

                // Wektory od zagłębienia do końców
                cv::Point2f a(
                    static_cast<float>(start.x - farPoint.x),
                    static_cast<float>(start.y - farPoint.y)
                );

                cv::Point2f b(
                    static_cast<float>(end.x - farPoint.x),
                    static_cast<float>(end.y - farPoint.y)
                );

                // Obliczamy długości wektorów
                double lenA = cv::norm(a);
                double lenB = cv::norm(b);

                if (lenA < 1.0 || lenB < 1.0)
                    continue;

                // Obliczamy kąt zagłębienia
                double cosine = (a.x * b.x + a.y * b.y) / (lenA * lenB);

                // Zabezpieczenie przed błędami zaokrągleń
                cosine = std::max(-1.0, std::min(1.0, cosine));

                double angle = std::acos(cosine) * 180.0 / CV_PI;

                // Odrzucamy zbyt szerokie kąty
                if (angle > 90.0)
                    continue;

                // Start i end są kandydatami na koniuszki palców
                cv::Point candidates[2] = {start, end};

                for (const cv::Point& candidate : candidates)
                {
                    bool duplicate = false;

                    // Sprawdzamy, czy punkt nie został już znaleziony
                    for (const cv::Point& existing : fingerTips)
                    {
                        if (cv::norm(candidate - existing) < 20.0)
                        {
                            duplicate = true;
                            break;
                        }
                    }

                    // Dodajemy tylko nowy punkt
                    if (!duplicate){
                        fingerTips.push_back(candidate);
                    }
                }
            }


            // Rysujemy znalezione punkty
            for (const cv::Point& tip : fingerTips)
            {
                cv::circle(
                    displayFrame,
                    tip,
                    7,
                    cv::Scalar(0, 0, 255),
                    -1
                );
            }

            // Wyświetlamy liczbę znalezionych kandydatów
            cv::putText(
                displayFrame,
                "Finger tips: " + std::to_string(fingerTips.size()),
                cv::Point(30, 40),
                cv::FONT_HERSHEY_SIMPLEX,
                0.8,
                cv::Scalar(0, 255, 255),
                2
            );
        }
            // Wyświetlamy obrazy pomocnicze
            cv::imshow("Gray", gray);
            cv::imshow("Binary", binary);
            cv::imshow("HSV Mask", mask);
            cv::imshow("FingerBot", displayFrame);

            // Odczytujemy naciśnięty klawisz
            int key = cv::waitKey(1);

            // Esc lub C — zamknięcie programu
            if (key == 27 || key == 'c' || key == 'C')
            {
                break;
            }

            // M — włączenie lub wyłączenie odbicia lustrzanego
            if (key == 'm' || key == 'M')
            {
                mirror = !mirror;
            }

    }

        // Zwalniamy kamerę
        camera.release();

        // Zamykamy wszystkie okna OpenCV
        cv::destroyAllWindows();

        return 0;
    }
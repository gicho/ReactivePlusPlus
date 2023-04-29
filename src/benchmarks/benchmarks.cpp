#include <nanobench.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string_view>

#include <rpp/rpp.hpp>
#ifdef RPP_BUILD_RXCPP
    #include <rxcpp/rx.hpp>
#endif

#define BENCHMARK(NAME)     bench.context("benchmark_title", NAME);
#define SECTION(NAME)       bench.context("benchmark_name", NAME);
#define TEST_RPP(ACTION)    bench.context("source", "rpp").run(ACTION);
#ifdef RPP_BUILD_RXCPP
    #define TEST_RXCPP(ACTION)  bench.context("source", "rxcpp").run(ACTION);
#else
    #define TEST_RXCPP(ACTION)
#endif

char const* json() noexcept {
    return R"DELIM([
{{#result}}        {
            "title": "{{context(benchmark_title)}}",
            "name": "{{context(benchmark_name)}}",
            "source" : "{{context(source)}}",
            "median(elapsed)": {{median(elapsed)}},
            "medianAbsolutePercentError(elapsed)": {{medianAbsolutePercentError(elapsed)}}
        }{{^-last}},{{/-last}}
{{/result}}
])DELIM";
}


int main(int argc, char* argv[]) // NOLINT
{
    auto bench = ankerl::nanobench::Bench{}.output(nullptr).warmup(3);

    BENCHMARK("General")
    {
        SECTION("Subscribe empty callbacks to empty observable")
        {
            TEST_RPP([&]()
            {
                rpp::source::create<int>([&](auto&& observer)
                {
                    ankerl::nanobench::doNotOptimizeAway(observer);
                }).subscribe([](int){}, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::create<int>([&](auto&& observer)
                {
                    ankerl::nanobench::doNotOptimizeAway(observer);
                }).subscribe([](int){}, [](const std::exception_ptr&){}, [](){});
            });
        }

        SECTION("Subscribe empty callbacks to empty observable via pipe operator")
        {
            TEST_RPP([&]()
            {
                rpp::source::create<int>([&](auto&& observer)
                {
                    ankerl::nanobench::doNotOptimizeAway(observer);
                }) | rpp::operators::subscribe([](int){}, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::create<int>([&](auto&& observer)
                {
                    ankerl::nanobench::doNotOptimizeAway(observer);
                }) | rxcpp::operators::subscribe<int>([](int){}, [](const std::exception_ptr&){}, [](){});
            });
        }
    };

    BENCHMARK("Sources")
    {
        SECTION("from array of 1 - create + subscribe + immediate")
        {
            std::array<int, 1> vals{123};
            TEST_RPP([&]()
            {
                rpp::source::from_iterable(vals, rpp::schedulers::immediate{}).subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::iterate(vals, rxcpp::identity_immediate()).subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });
        }
        SECTION("from array of 1 - create + subscribe + current_thread")
        {
            std::array<int, 1> vals{123};
            TEST_RPP([&]()
            {
                rpp::source::from_iterable(vals, rpp::schedulers::current_thread{}).subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::iterate(vals, rxcpp::identity_current_thread()).subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });
        }
    };

    BENCHMARK("Schedulers")
    {
        SECTION("immediate scheduler create worker + schedule")
        {
            TEST_RPP([&]()
            {
                rpp::schedulers::immediate::create_worker().schedule([](const auto& v){ ankerl::nanobench::doNotOptimizeAway(v); return rpp::schedulers::optional_duration{}; }, rpp::make_lambda_observer([](int){ }));
            });
            TEST_RXCPP([&]()
            {
                rxcpp::identity_immediate().create_coordinator().get_worker().schedule([](const auto& v){ ankerl::nanobench::doNotOptimizeAway(v); });
            });
        }
        SECTION("current_thread scheduler create worker + schedule")
        {
            TEST_RPP([&]()
            {
                rpp::schedulers::current_thread::create_worker().schedule([](const auto& v){ ankerl::nanobench::doNotOptimizeAway(v); return rpp::schedulers::optional_duration{}; }, rpp::make_lambda_observer([](int){ }));
            });
            TEST_RXCPP([&]()
            {
                rxcpp::identity_current_thread().create_coordinator().get_worker().schedule([](const auto& v){ ankerl::nanobench::doNotOptimizeAway(v); });
            });
        }
        SECTION("current_thread scheduler create worker + schedule + recursive schedule")
        {
            TEST_RPP(
                [&]()
                {
                    const auto worker = rpp::schedulers::current_thread::create_worker();
                    worker.schedule(
                        [&worker](auto&& v)
                        {
                            worker.schedule(
                                [](const auto& v)
                                {
                                    ankerl::nanobench::doNotOptimizeAway(v);
                                    return rpp::schedulers::optional_duration{};
                                },
                                std::move(v));
                            return rpp::schedulers::optional_duration{};
                        },
                        rpp::make_lambda_observer([](int) {}));
                });
            TEST_RXCPP(
                [&]()
                {
                    const auto worker = rxcpp::identity_current_thread()
                        .create_coordinator()
                        .get_worker();

                    worker.schedule([&worker](const auto&)
                                    { 
                                        worker.schedule([](const auto& v) { ankerl::nanobench::doNotOptimizeAway(v); }); 
                                    });
                });
        }
    }

    BENCHMARK("Transforming Operators")
    {
        SECTION("create+map(v*2)+subscribe")
        {
            TEST_RPP([&]()
            {
                rpp::source::create<int>([](const auto& obs){ obs.on_next(1); })
                    | rpp::operators::map([](int v){return v*2; })
                    | rpp::operators::subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::create<int>([](const auto& obs){obs.on_next(1);})
                    | rxcpp::operators::map([](int v){return v*2;})
                    | rxcpp::operators::subscribe<int>([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });
        }
    };

    BENCHMARK("Filtering Operators")
    {
        SECTION("create+take(1)+subscribe")
        {
            TEST_RPP([&]()
            {
                rpp::source::create<int>([](const auto& obs){ obs.on_next(1); })
                    | rpp::operators::take(1)
                    | rpp::operators::subscribe([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });

            TEST_RXCPP([&]()
            {
                rxcpp::observable<>::create<int>([](const auto& obs){obs.on_next(1);})
                    | rxcpp::operators::take(1)
                    | rxcpp::operators::subscribe<int>([](int v){ ankerl::nanobench::doNotOptimizeAway(v); }, [](const std::exception_ptr&){}, [](){});
            });
        }
    };

    if (argc > 1) {
        std::ofstream of{argv[1]};
        bench.render(json(), of);
        of.close();
    }

    bench.render(json(), std::cout);
}
